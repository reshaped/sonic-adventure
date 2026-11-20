/* the PC's raster draws a picture at a time with Vulkan, headless or through
 * a window's swapchain. a frame is the buffers of one picture in flight. a
 * picture is placed in the pixels of a 640 by 480 screen and drawn at
 * RasterOptions' resolution. a batch is a run of triangles under one State */
#ifndef ENGINE_PC_RASTER_H
#define ENGINE_PC_RASTER_H

#include <stdint.h>

#include <string>
#include <vector>

#include "pvr.h"
#include "sa_engine.h"
#include "window.h"

namespace pc {

constexpr uint32_t WIDTH = 640;
constexpr uint32_t HEIGHT = 480;

/* a corner. at is in world space, or under BATCH_SCREEN in pixels with its
 * view z. the colors are RGBA in memory */
struct Vertex {
    float at[3];
    float uv[2];
    uint32_t base;
    uint32_t offset;
};

/* the batch's flags the shaders read. the fog mode, an SA_FOG_ value, takes
 * the two bits from BATCH_FOG_SHIFT */
constexpr uint32_t BATCH_TEXTURE = 1;
constexpr uint32_t BATCH_DECAL = 2;
constexpr uint32_t BATCH_OFFSET = 4;
constexpr uint32_t BATCH_OPAQUE = 8;
constexpr uint32_t BATCH_SCREEN = 16;
constexpr uint32_t BATCH_FOG_SHIFT = 5;
/* the batch's fragments go to per-pixel lists, which resolve.comp sorts */
constexpr uint32_t BATCH_COLLECT = 128;
/* the batch clips at the picture's near_z, as Ninja's full drawer does.
 * other batches clip at z = 1 */
constexpr uint32_t BATCH_NEAR = 256;

/* how a batch samples its texture. the low two bits hold an SA_FILTER_
 * value. SAMPLE_ADJUST holds the mipmap's D adjust as a material's mip_bias
 * does, in quarters */
constexpr uint32_t SAMPLE_FILTER = 3;
constexpr uint32_t SAMPLE_CLAMP_U = 4;
constexpr uint32_t SAMPLE_CLAMP_V = 8;
constexpr uint32_t SAMPLE_FLIP_U = 16;
constexpr uint32_t SAMPLE_FLIP_V = 32;
constexpr uint32_t SAMPLE_ADJUST_SHIFT = 6;
constexpr uint32_t SAMPLE_ADJUST = 15U << SAMPLE_ADJUST_SHIFT;
/* a D adjust of 1.0, which leaves the mipmap level as the texels a pixel
 * spans give it. sa_sprite and sa_triangles draws take it, as their records
 * hold no adjust */
constexpr uint32_t ADJUST_ONE = 4;
/* a filter and a D adjust as State's sample holds them. a filter past
 * trilinear is held to it, so a sample names no filter a sampler lacks */
constexpr uint32_t sample_of(uint32_t filter, uint32_t adjust)
{
    constexpr uint32_t last = SA_FILTER_TRILINEAR;
    uint32_t held = filter < last ? filter : last;

    return held | (adjust & 15U) << SAMPLE_ADJUST_SHIFT;
}

struct State {
    uint32_t flags = 0; /* BATCH_* and the fog mode */
    uint32_t texture = 0; /* a texture's index, 0 for none */
    uint32_t sample = 0; /* SAMPLE_* */
    uint8_t blend = 0; /* 1 where the batch blends */
    /* SA_BLEND_*, the PowerVR's blend instructions */
    uint8_t src = SA_BLEND_ONE;
    uint8_t dst = SA_BLEND_ZERO;
    /* SA_DEPTH_*, the PowerVR's compare modes, on 1/z */
    uint8_t depth = SA_DEPTH_GEQUAL;
    uint8_t depth_write = 1;
    /* 1 drops a triangle that turns clockwise on the screen */
    uint8_t cull = 0;
    uint16_t reserved = 0; /* no padding, so a state compares by its bytes */
};

struct Batch {
    State state;
    uint32_t first = 0;
    uint32_t count = 0;
};

/* a picture's constants: its view, projection and fog */
struct PictureUniform {
    float view[12];
    float project[4]; /* focal x and y, then center x and y */
    float fog_color[4];
    float fog[4]; /* the density, then the picture's near_z */
    float table[256];
};

struct RasterOptions {
    std::string device; /* the first listed whose name holds this */
    Window *window = nullptr; /* null for headless */
    uint32_t frames = 1; /* the pictures in flight */
    bool vsync = true;
    bool readback = false; /* keep each picture for read */
    /* the picture is drawn at WIDTH by HEIGHT times this. 0 takes the size
     * the window shows it at, never under WIDTH by HEIGHT, and is 1
     * headless */
    uint32_t resolution = 1;
};

/* a call that fails says why, naming any Vulkan call that failed and how.
 * one that fails making a thing keeps nothing half made, so any but open may
 * be made again. a failed send of textures or a picture breaks the raster,
 * whose device may hold part of it. a failed wait and a lost device break it
 * too. a broken raster refuses each texture and picture after with its first
 * reason. none of its frames gives a picture, and it ends as any other
 * raster does */
class Raster {
public:
    Raster() = default;
    Raster(const Raster &) = delete;
    Raster &operator=(const Raster &) = delete;
    ~Raster();

    /* make the device: the first listed whose name holds options' device,
     * or else the first listed. a raster opens once: a second open returns
     * false, whether or not the first took */
    bool open(const RasterOptions &options, std::string &why);
    std::string device_name() const;
    /* make a texture of image for the batches. it returns its index, from 1,
     * or 0 where the device refuses. texture 0 is white */
    uint32_t add_texture(const Image &image, std::string &why);
    /* end texture once no frame in flight reads it. a broken raster keeps
     * its textures until it ends */
    void free_texture(uint32_t texture);
    /* draw the batches over clear, an ARGB color, on frame, and show the
     * picture in any window. it waits first for frame's last picture to
     * retire. where sorted, a blending batch's fragments compose per pixel
     * from the farthest, as the PowerVR's autosort does. else they blend in
     * the batches' order */
    bool submit(uint32_t frame, const PictureUniform &picture, uint32_t clear,
                const std::vector<Vertex> &vertices,
                const std::vector<Batch> &batches, bool sorted,
                std::string &why);
    /* show the pictures from the next submit at 16:9 where on, else at 4:3.
     * a window shows them in its largest centered rectangle of that aspect.
     * the picture keeps its WIDTH by HEIGHT coordinates */
    void wide(bool on);
    /* true once the raster is broken, with why, its first reason */
    bool broken(std::string &why) const;
    /* true once frame's picture has retired. a frame never used, and one
     * whose picture the device refused or lost, counts as retired */
    bool done(uint32_t frame);
    void wait(uint32_t frame);
    /* lost gives the blend fragments of frame's retired picture that the
     * per-pixel lists had no room for. cut gives those a pixel held past the
     * ones it composes. gpu_ns gives the device's time on the picture, 0
     * where it can't say. each gives 0 for a frame that holds no picture and
     * on a broken raster */
    uint32_t lost(uint32_t frame);
    uint32_t cut(uint32_t frame);
    uint64_t gpu_ns(uint32_t frame);
    /* copy frame's retired picture into rgba, WIDTH by HEIGHT RGBA by rows
     * at any resolution. it returns false, and leaves rgba alone, without
     * options' readback. it does the same for a frame that holds no picture
     * and on a broken raster */
    bool read(uint32_t frame, void *rgba);

private:
    struct Impl;
    Impl *impl_ = nullptr;
};

} // namespace pc

#endif
