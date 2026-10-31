/* YuraYura, the land's moving snow: it steps the texture of the snow's
 * materials through 15 images */
#include <ninja.h>
#include <sg_xpt.h>

#include "stg08/icecap02_textures.h"
#include "stg08/yura_yura.h"
#include "system/task.h"

/* the land's materials that show the moving snow */
extern NJS_MATERIAL dat_0c977dac[1];
extern NJS_MATERIAL dat_0c978228[1];
extern NJS_MATERIAL dat_0c9830c8[2];
extern NJS_MATERIAL dat_0c98f8c4[1];
extern NJS_MATERIAL dat_0c98f9c8[1];
extern NJS_MATERIAL dat_0c9a5eec[1];
extern NJS_MATERIAL dat_0c9a8720[2];
extern NJS_MATERIAL dat_0c9aa3fc[1];
extern NJS_MATERIAL dat_0c9aaabc[3];
extern NJS_MATERIAL dat_0c9ac498[1];
extern NJS_MATERIAL dat_0c9ad698[1];
extern NJS_MATERIAL dat_0c9b7230[2];
extern NJS_MATERIAL dat_0c9b94ac[1];
extern NJS_MATERIAL dat_0c9bda40[2];
extern NJS_MATERIAL dat_0c9be30c[2];
extern NJS_MATERIAL dat_0c9c57e4[1];
extern NJS_MATERIAL dat_0c9c5924[1];
extern NJS_MATERIAL dat_0c9c5a28[1];
extern NJS_MATERIAL dat_0c9c5b68[1];
extern NJS_MATERIAL dat_0c9c5c6c[1];

#define YURA_YURA_IMAGES 15
#define YURA_YURA_DELAY 4 /* the steps each image shows for */

struct yura_yura_vars {
    Uint32 image;
};

#define VARS(work) ((struct yura_yura_vars *)&(work)->vars)

/* each image's texture, by its index in icecap02_texlist: mo128_snow001,
 * then mo128_snow002 to mo128_snow015 */
static Uint32 yura_yura_textures[YURA_YURA_IMAGES] = {
#include "stg08/yura_yura/yura_yura_textures.inc"
};

static void yura_yura_run(struct task *task)
{
    if (++task->work->tick == YURA_YURA_DELAY) {
        task->work->tick = 0;
        VARS(task->work)->image =
            (VARS(task->work)->image + 1) % YURA_YURA_IMAGES;
        /* clang-format off */
        /* @hack spelling: the materials' textures set in one chain, which has SHC
         * take their addresses first, twelve of them onto the stack
         * (0x0c9103a2 to 0x0c910412), and read the texture once, at
         * 0x0c91041c; as a statement for each, SHC reads the texture again
         * for each store, the function runs 64 bytes long and 144 of 144
         * words differ */
        dat_0c98f8c4[0].attr_texId =
        dat_0c98f9c8[0].attr_texId =
        dat_0c977dac[0].attr_texId =
        dat_0c978228[0].attr_texId =
        dat_0c9830c8[0].attr_texId =
        dat_0c9830c8[1].attr_texId =
        dat_0c9c57e4[0].attr_texId =
        dat_0c9c5924[0].attr_texId =
        dat_0c9c5a28[0].attr_texId =
        dat_0c9c5b68[0].attr_texId =
        dat_0c9c5c6c[0].attr_texId =
        dat_0c9a5eec[0].attr_texId =
        dat_0c9a8720[0].attr_texId =
        dat_0c9a8720[1].attr_texId =
        dat_0c9aa3fc[0].attr_texId =
        dat_0c9aaabc[0].attr_texId =
        dat_0c9aaabc[1].attr_texId =
        dat_0c9aaabc[2].attr_texId =
        dat_0c9ac498[0].attr_texId =
        dat_0c9ad698[0].attr_texId =
        dat_0c95cb4c[0].attr_texId =
        dat_0c9b7230[0].attr_texId =
        dat_0c9b7230[1].attr_texId =
        dat_0c9b94ac[0].attr_texId =
        dat_0c9bda40[0].attr_texId =
        dat_0c9bda40[1].attr_texId =
        dat_0c9be30c[0].attr_texId =
        dat_0c9be30c[1].attr_texId =
            yura_yura_textures[VARS(task->work)->image];
        /* clang-format on */
    }
}

void yura_yura_step(struct task *task)
{
    VARS(task->work)->image = 0;
    task->work->tick = 0;
    task->step = yura_yura_run;
}
