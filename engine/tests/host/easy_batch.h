/* engine/dc/batch.h for easy_sends.c: what the easy walk reads of it.
 * batch.h's kernels are the SH-4's own instructions, which no host compiles.
 * this file declares the screen, easy_project's two kernels, and the store
 * queues' registers and addresses easy_open uses. the test calls neither
 * easy_project nor easy_open. its sends read only the screen */
#ifndef EASY_BATCH_H
#define EASY_BATCH_H

/* batch.h's guard, so the walk's include of it reads none of it */
#define BATCH_H

#include <stdint.h>
#include <sh4zam/shz_vector.h>

typedef struct zam_screen {
    float cx, cy;
    float scale_x, scale_y;
    float near;
} zam_screen_t;
shz_vec3_t zam_xmtrx_transform_vec3_transpose(const float *v);
int zam_xmtrx_project_lit_points(const float *points, const float *normals,
                                 uint32_t count, const zam_screen_t *screen,
                                 shz_vec4_t light, void *out);
#define ZAM_QACR0 (*(volatile uint32_t *)0xff000038)
#define ZAM_QACR1 (*(volatile uint32_t *)0xff00003c)
#define ZAM_SQ_AREA(address) (((address) >> 24) & 0x1c)
#define ZAM_SQ_ADDRESS 0x03ffffff
#define ZAM_SQ_BASE 0xe0000000

#endif
