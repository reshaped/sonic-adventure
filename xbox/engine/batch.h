/* the Xbox port's engine/dc/batch.h, for the easy models' walk
 * (engine/dc/sa1/easy.h), which finds this one by name: the screen the
 * kernels project onto, the store queues by the port's stand-ins, and
 * zam_xmtrx_project_lit_points in C, on the port's models of FTRV and FIPR
 * (xbox/include/sh4.h), where the Dreamcast's is SH-4 assembly. the port
 * maps the store queues' area on RAM, so a store there writes the list */
#ifndef BATCH_H
#define BATCH_H

#include <zam.h>
#include <stdint.h>
#include <sh4zam/shz_vector.h>
#include <sh4zam/shz_xmtrx.h>

#define ZAM_PREFETCH_AHEAD 64

/* the screen the kernels project onto. scale_x and scale_y take a view-space
 * x / z or y / z to the screen, about cx and cy. near is the z no point at
 * or before is drawn by */
typedef struct zam_screen {
    float cx, cy;
    float scale_x, scale_y;
    float near;
} zam_screen_t;

/* the store queues. the Xbox's flash ROM is at 0xff000000, so the two
 * registers are xbox/port/system.c's, which nothing reads */
extern volatile Uint32 port_qacr[2];
#define ZAM_QACR0 (port_qacr[0])
#define ZAM_QACR1 (port_qacr[1])
#define ZAM_SQ_AREA(address) (((address) >> 24) & 0x1c)
#define ZAM_SQ_ADDRESS 0x03ffffff
#define ZAM_SQ_BASE 0xe0000000

/* the Dreamcast's kernel, in C: project count points, three floats each,
 * by XMTRX onto screen, and light each by its normal at normals. each point
 * takes 16 bytes of out: its x and y on the screen, 1/z and its intensity.
 * x * scale_x * (1/z) + cx gives x on the screen, rounded in that order, and
 * y alike by scale_y and cy. the intensity is FIPR's dot of light with the
 * normal, given a w of 1, or light's w where the dot is not over it. return
 * how many points lie no farther than screen->near, by the z XMTRX gives */
ALWAYS_INLINE int zam_xmtrx_project_lit_points(const float *points,
                                               const float *normals,
                                               uint32_t count,
                                               const zam_screen_t *screen,
                                               shz_vec4_t light, void *out)
{
    float *to = out;
    uint32_t beyond = 0;
    uint32_t k;

    for (k = 0; k < count; k++, points += 3, normals += 3, to += 4) {
        float v[4] = {points[0], points[1], points[2], 1.0F};
        float dot = sh4_fipr(light.x, light.y, light.z, light.w, normals[0],
                             normals[1], normals[2], 1.0F);
        float rhw;

        sh4_xmtrx_ftrv(v, v);
        if (v[2] > screen->near) {
            beyond++;
        }
        rhw = 1.0F / v[2];
        to[0] = v[0] * screen->scale_x * rhw + screen->cx;
        to[1] = v[1] * screen->scale_y * rhw + screen->cy;
        to[2] = rhw;
        to[3] = dot > light.w ? dot : light.w;
    }
    return (int)(count - beyond);
}

#endif
