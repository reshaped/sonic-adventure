/* EV00B7.PRS's dimming: a task that darkens the models' palettes and puts
 * them back */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/model.h"
#include "ev00b7/dim.h"
#include "system/task.h"

/* how many of draw_palettes the dimming darkens, from the first */
#define DIM_PALETTES 2

/* the light's diffuse and ambient intensities, each ten times the value
 * njSetLightIntensity takes */
static Sint32 dim_diffuse = 0;
static Sint32 dim_ambient = -30;
static struct task *dim_task = NULL;

static NJS_LIGHT dim_light;
static Uint32 dim_saved[DIM_PALETTES][DRAW_PALETTE_ENTRIES * 2];

static void ev00b7_dim_destroy(struct task *task)
{
    int palette;
    int entry;

    njDeleteLight(&dim_light);
    for (palette = 0; palette < DIM_PALETTES; palette++) {
        for (entry = 0; entry < DRAW_PALETTE_ENTRIES; entry++) {
            draw_palettes[palette][entry * 2] = dim_saved[palette][entry * 2];
            draw_palettes[palette][(entry * 2) + 1] =
                dim_saved[palette][(entry * 2) + 1];
        }
    }
}

static void ev00b7_dim_draw(struct task *task)
{
}

static void ev00b7_dim_step(struct task *task)
{
}

static void ev00b7_dim_init(struct task *task)
{
    int palette;
    int entry;
    /* @hack type: color signed, which SHC shifts with SHAR (0x0cb80fae,
     * 0x0cb80fd4), though the mask clears the bit SHAR copies; as a Uint32,
     * it shifts with SHLR and 2 of 123 words differ */
    Sint32 color;

    njCreateLight(&dim_light, NJD_SIMPLE_LIGHT | NJD_AMBIENT | NJD_DIR_LIGHT);
    njSetLightIntensity(&dim_light, 0.0F, dim_diffuse * 0.1F,
                        dim_ambient * 0.1F);
    njSetLightColor(&dim_light, 1.0F, 1.0F, 1.0F);
    njSetLightDirection(&dim_light, 0.0F, 0.0F, 1.0F);
    task->step = ev00b7_dim_step;
    task->draw = ev00b7_dim_draw;
    task->destructor = ev00b7_dim_destroy;
    for (palette = 0; palette < DIM_PALETTES; palette++) {
        for (entry = 0; entry < DRAW_PALETTE_ENTRIES; entry++) {
            /* each base color drops to half its brightness and becomes
             * opaque. each offset color becomes black, which adds
             * nothing */
            color = draw_palettes[palette][entry * 2];
            dim_saved[palette][entry * 2] = color;
            draw_palettes[palette][entry * 2] =
                ((color >> 1) & 0x007f7f7f) | 0xff000000;
            color = draw_palettes[palette][(entry * 2) + 1];
            dim_saved[palette][(entry * 2) + 1] = color;
            draw_palettes[palette][(entry * 2) + 1] = 0xff000000;
        }
    }
}

void ev00b7_dim_create(void)
{
    dim_task = task_create(0, 1, ev00b7_dim_init);
}

void ev00b7_dim_kill(void)
{
    if (dim_task != NULL) {
        task_kill(dim_task);
        dim_task = NULL;
    }
}
