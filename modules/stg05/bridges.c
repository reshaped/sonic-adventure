/* O TURI1 to O TURI4, rope bridges. each one's first step hands its parts to
 * bridge_init, which builds the bridge and takes over the task */
#include <ninja.h>
#include <sg_xpt.h>

#include "object/bridge.h"
#include "stg05/bridges.h"
#include "system/task.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_MODEL mountain_bridge_1_model;
extern NJS_MODEL mountain_bridge_2_model;
extern NJS_MODEL mountain_bridge_3_model;
extern NJS_MODEL mountain_bridge_4_model;
extern NJS_MODEL mountain_bridge_post_model;
extern NJS_MODEL mountain_bridge_rope_model;
extern NJS_MODEL mountain_bridge_plank_model;

/* the first nine of bridge_init's default params. struct bridge_params holds
 * ten, but mountain_bridge_1_parts follows the ninth in the image. a bridge
 * reads only width, the eighth */
static Float mountain_bridge_params[9] = {
    170.0F, 2.5F, -0.12F, 16.0F, 11.5F, 0.04F, 0.0065F, 17.5F, -1.3F,
};

#define PARAMS ((struct bridge_params *)mountain_bridge_params)

static struct bridge_parts mountain_bridge_1_parts = {
    {{0, 1, 2, 3}, {4, 5, 6, 7}},
    {{124, 125, 126, 127}, {128, 129, 130, 131}},
    {0, 1, 2, 3},
    {{0.0F, 182.726F}, {-30.0F, 72.0F}, {-17.7F, 17.7F}},
    182.726F,
    1.0F,
    &obj_mountain_texlist,
    &mountain_bridge_1_model,
    &mountain_bridge_post_model,
    &mountain_bridge_rope_model,
    &mountain_bridge_plank_model,
    PARAMS,
};
static struct bridge_parts mountain_bridge_2_parts = {
    {{0, 1, 2, 3}, {4, 5, 6, 7}},
    {{132, 133, 134, 135}, {136, 137, 138, 139}},
    {0, 1, 2, 3},
    {{0.0F, 194.549F}, {-50.0F, 72.0F}, {-17.7F, 17.7F}},
    194.549F,
    1.0F,
    &obj_mountain_texlist,
    &mountain_bridge_2_model,
    &mountain_bridge_post_model,
    &mountain_bridge_rope_model,
    &mountain_bridge_plank_model,
    PARAMS,
};
static struct bridge_parts mountain_bridge_3_parts = {
    {{0, 1, 2, 3}, {4, 5, 6, 7}},
    {{120, 121, 122, 123}, {124, 125, 126, 127}},
    {0, 1, 2, 3},
    {{0.0F, 176.81F}, {-50.0F, 72.0F}, {-17.7F, 17.7F}},
    176.81F,
    1.0F,
    &obj_mountain_texlist,
    &mountain_bridge_3_model,
    &mountain_bridge_post_model,
    &mountain_bridge_rope_model,
    &mountain_bridge_plank_model,
    PARAMS,
};
static struct bridge_parts mountain_bridge_4_parts = {
    {{0, 1, 2, 3}, {4, 5, 6, 7}},
    {{108, 109, 110, 111}, {112, 113, 114, 115}},
    {0, 1, 2, 3},
    {{0.0F, 159.07F}, {-50.0F, 72.0F}, {-17.7F, 17.7F}},
    159.07F,
    1.0F,
    &obj_mountain_texlist,
    &mountain_bridge_4_model,
    &mountain_bridge_post_model,
    &mountain_bridge_rope_model,
    &mountain_bridge_plank_model,
    PARAMS,
};

void mountain_bridge_1_step(struct task *task)
{
    BRIDGE_VARS(task->work)->parts = &mountain_bridge_1_parts;
    bridge_init(task);
}

void mountain_bridge_2_step(struct task *task)
{
    BRIDGE_VARS(task->work)->parts = &mountain_bridge_2_parts;
    bridge_init(task);
}

void mountain_bridge_3_step(struct task *task)
{
    BRIDGE_VARS(task->work)->parts = &mountain_bridge_3_parts;
    bridge_init(task);
}

void mountain_bridge_4_step(struct task *task)
{
    BRIDGE_VARS(task->work)->parts = &mountain_bridge_4_parts;
    bridge_init(task);
}
