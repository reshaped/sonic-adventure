/* E-102's hit: a flash, and a burst of sparks, on E102HIT's textures */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "effect/sprite.h"
#include "effect/sprite_pool.h"
#include "player/e102_hit.h"
#include "system/task.h"
#include "system/util.h"

#define POOL_SPARKS 40 /* the sparks the pool has room for */
#define BURST_SPARKS 20 /* the sparks a burst adds */

/* E102HIT's textures, with empty names: only its PVM names them */
static const NJS_TEXNAME e102hit_textures[17] = {
    {"", 0, 0}, {"", 0, 0}, {"", 0, 0}, {"", 0, 0}, {"", 0, 0}, {"", 0, 0},
    {"", 0, 0}, {"", 0, 0}, {"", 0, 0}, {"", 0, 0}, {"", 0, 0}, {"", 0, 0},
    {"", 0, 0}, {"", 0, 0}, {"", 0, 0}, {"", 0, 0}, {"", 0, 0},
};

/* the flash: textures 1 to 16, two steps each */
static const struct sprite_frame hit_flash_frames[17] = {
#include "player/e102_hit/hit_flash_frames.inc"
};

/* a spark: texture 0, going from white through orange to a dark red as
 * it fades */
static const struct sprite_frame hit_spark_frames[5] = {
#include "player/e102_hit/hit_spark_frames.inc"
};

NJS_TEXLIST e102hit_texlist = {
#include "player/e102_hit/e102hit_texlist.inc"
};

/* the sparks' pool, or NULL while there's none */
static struct task *hit_sparks = NULL;

static void hit_sparks_create(void)
{
    struct sprite_motion motion;

    if (hit_sparks == NULL) {
        sprite_bind(&e102hit_texlist, hit_spark_frames, 0);
        motion.drag = 0.03F;
        motion.gravity = 0.0F;
        hit_sparks =
            sprite_pool_create(POOL_SPARKS, &motion, hit_spark_frames, NULL);
    }
}

/* each spark flies off at -2 to 2 on each axis, from a random age of 0 to
 * 15 */
static void hit_burst(NJS_POINT3 *pos, Float depth)
{
    NJS_VECTOR speed;
    int i;

    if (hit_sparks != NULL) {
        for (i = 0; i < BURST_SPARKS; i++) {
            speed.x = (rand() * RAND_SCALE * 4.0F) - 2.0F;
            speed.y = (rand() * RAND_SCALE * 4.0F) - 2.0F;
            speed.z = (rand() * RAND_SCALE * 4.0F) - 2.0F;
            sprite_pool_add(hit_sparks, pos, &speed,
                            rand() * RAND_SCALE * 15.0F, depth);
        }
    }
}

void e102_hit_init(void)
{
    sprite_bind(&e102hit_texlist, hit_flash_frames, 0);
    hit_sparks_create();
}

void e102_hit_free(void)
{
    if (hit_sparks != NULL) {
        task_kill(hit_sparks);
        hit_sparks = NULL;
    }
}

void e102_hit_create(NJS_POINT3 *pos, Float depth)
{
    struct sprite_desc desc;

    desc.step = NULL;
    desc.speed.x = desc.speed.y = desc.speed.z = 0.0F;
    desc.scale = 1.0F;
    desc.depth = depth;
    desc.drag = 0.0F;
    desc.gravity = 0.0F;
    desc.frames = hit_flash_frames;
    desc.rate = 1.0F;
    sprite_create(pos, &desc, SPRITE_MOVES);
    hit_burst(pos, depth);
}
