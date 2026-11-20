/* the PSP's scene, for what SA1's adapter draws that is no model (draw.h):
 * sprites and runs of triangles, a few hundred a picture. a pc::Scene makes
 * their corners, as it does a host's, and the raster takes them at the
 * picture's end. the adapter hands each over as it would record it, so no
 * record, client or engine's resource lies between */
#include <pspkernel.h>
#include <stdio.h>
#include <string.h>

#include "draw.h"
#include "ninja.h"
#include "raster.h"
#include "scene.h"

namespace pc {
/* raster.cpp's */
bool psp_submit(const PictureUniform &picture, uint32_t clear,
                const std::vector<Vertex> &vertices,
                const std::vector<Batch> &batches, bool sorted,
                const std::vector<float> *places);
/* the microseconds its pictures waited for the display's vertical blank */
extern uint32_t psp_wait_us;
} // namespace pc

namespace {

/* the pictures shown since the last report, the microseconds from each to
 * the next, and those the walk took */
uint32_t frames, frames_us, frame_began, walk_us, walk_began;

constexpr uint32_t NO_VIEW = UINT32_MAX;

pc::Scene scene;
bool opened, begun;
/* the scene's index of each of the adapter's views, and of the picture's
 * own at 0 */
uint32_t views[SA_VIEWS + 1];
/* the draws a run left out for want of a view, a texture or room, and the
 * most triangles a picture's took */
uint32_t lost_view, lost_texture, lost_room, most_triangles;

/* the scene's view of a draw under the adapter's view id, or NO_VIEW */
uint32_t view_of(uint32_t id)
{
    return id <= SA_VIEWS ? views[id] : NO_VIEW;
}

/* count a draw left out, by what it wanted, and say whether it is */
bool left_out(uint32_t view, uint32_t texture, uint32_t triangles)
{
    if (!begun || view == NO_VIEW) {
        lost_view++;
    } else if (texture == PSP_TEXTURE_FAILED) {
        lost_texture++;
    } else if (!scene.room(triangles)) {
        lost_room++;
    } else {
        return false;
    }
    return true;
}

} // namespace

extern "C" void psp_scene_begin(const sa_view *view, const uint32_t back[3])
{
    sa_picture picture{};

    if (!opened) {
        /* what blends goes the farthest first, by the triangle: the engine
         * has no order by the pixel, as the PowerVR's is */
        scene.open(SA_PC_BLEND_TRIANGLE);
        opened = true;
    }
    picture.head = {SA_REC_PICTURE, sizeof picture};
    picture.view = *view;
    picture.back[0] = back[0];
    picture.back[1] = back[1];
    picture.back[2] = back[2];
    scene.begin(picture);
    views[0] = 0;
    for (uint32_t i = 1; i <= SA_VIEWS; i++) {
        views[i] = NO_VIEW;
    }
    begun = true;
}

extern "C" void psp_scene_view(uint32_t id, const sa_view *view)
{
    if (begun && id >= 1 && id <= SA_VIEWS && views[id] == NO_VIEW) {
        views[id] = scene.view(*view);
    }
}

extern "C" void psp_scene_sprite(const sa_sprite *sprite)
{
    bool screen = (sprite->flags & SA_SPRITE_SCREEN) != 0;
    uint32_t view = screen ? 0 : view_of(sprite->view);

    if (left_out(view, sprite->texture, 2)) {
        return;
    }
    scene.sprite(*sprite,
                 sprite->texture == PSP_NO_TEXTURE ? 0 : sprite->texture, view);
}

extern "C" void psp_scene_triangles(const sa_triangles *triangles,
                                    const sa_point *points)
{
    bool screen = (triangles->flags & SA_TRIANGLES_SCREEN) != 0;
    uint32_t view = screen ? 0 : view_of(triangles->view);

    if (left_out(view, triangles->texture,
                 triangles->form == SA_TRIANGLES_LINES ? triangles->count * 2U
                                                       : triangles->count)) {
        return;
    }
    scene.triangles(
        *triangles, points,
        triangles->texture == PSP_NO_TEXTURE ? 0 : triangles->texture, view);
}

extern "C" void psp_scene_end(uint32_t clear, const sa_view *own)
{
    pc::PictureUniform uniform;

    if (!begun) {
        return;
    }
    begun = false;
    most_triangles = scene.triangle_count() > most_triangles
                         ? scene.triangle_count()
                         : most_triangles;
    scene.finish();
    scene.uniform(uniform);
    if (own != nullptr) {
        uniform.project[0] = own->focal[0];
        uniform.project[1] = own->focal[1];
        uniform.project[2] = own->center[0];
        uniform.project[3] = own->center[1];
    }
    /* a blend writes no depth, so one drawn before another behind it hides
     * none of it */
    pc::psp_submit(uniform, clear, scene.vertices(), scene.batches(), true,
                   nullptr);
    uint32_t now = sceKernelGetSystemTimeLow();

    /* the log's word that the run reached its first picture */
    if (frame_began == 0) {
        fprintf(stderr, "sa1: the first picture is shown\n");
    }
    if (frame_began != 0) {
        frames++;
        frames_us += now - frame_began;
    }
    frame_began = now != 0 ? now : 1;
}

extern "C" void psp_walk_begin(void)
{
    walk_began = sceKernelGetSystemTimeLow();
}

extern "C" void psp_walk_end(void)
{
    walk_us += sceKernelGetSystemTimeLow() - walk_began;
}

extern "C" void psp_frame_report(void)
{
    if (frames != 0) {
        fprintf(stderr,
                "sa1: %u pictures, %u us each: %u waiting for the display, "
                "%u in the walk\n",
                static_cast<unsigned>(frames),
                static_cast<unsigned>(frames_us / frames),
                static_cast<unsigned>(pc::psp_wait_us / frames),
                static_cast<unsigned>(walk_us / frames));
    }
    frames = frames_us = walk_us = 0;
    pc::psp_wait_us = 0;
}

extern "C" void psp_ninja_lit(const sa_lights *lights, const void *material,
                              const float point[3], const float normal[3],
                              uint32_t *base, uint32_t *offset)
{
    sa_material held;

    memcpy(&held, material, sizeof held);
    pc::ninja_lit(*lights, held, point, normal, *base, *offset);
}

extern "C" void psp_scene_report(void)
{
    fprintf(stderr,
            "sa1: the scene's sprites and triangles took at most %u "
            "triangles, and %u draws were left out: %u for a view, %u for a "
            "texture, %u for room\n",
            static_cast<unsigned>(most_triangles),
            static_cast<unsigned>(lost_view + lost_texture + lost_room),
            static_cast<unsigned>(lost_view),
            static_cast<unsigned>(lost_texture),
            static_cast<unsigned>(lost_room));
}
