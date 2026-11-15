/* the services the port layer takes from its host: the Xbox's XBE, or a
 * native program on a PC. the host fills a struct host_api and stores its
 * address at HOST_API_SLOT, in the console's first 64 KB, which 1ST_READ.BIN
 * starts past and the port leaves free, before it runs the game's main.
 * every call is cdecl, as both sides compile it */
#ifndef XBOX_PORT_HOST_H
#define XBOX_PORT_HOST_H

#define HOST_API_SLOT 0x0c000100
#define HOST_API_VERSION 19

/* a vertex of a triangle the renderer draws: x and y in the console's 640
 * by 480 pixels, rhw the PowerVR's 1/w, which grows toward the eye, u and v
 * in the texture's own 0 to 1, and the base and offset colors as ARGB */
struct host_vertex {
    float x, y, rhw, u, v;
    unsigned base, offset;
};

/* how a batch of triangles draws, in the PowerVR's own terms */
struct host_draw {
    unsigned texture; /* host_api's video_texture id, or 0 for none */
    unsigned char src_blend, dst_blend; /* the TSP's instructions, 0 to 7 */
    unsigned char depth_compare; /* the ISP's, 0 never to 7 always, on rhw */
    unsigned char depth_write;
    unsigned char shading; /* the TSP's texture/shading instruction, 0 to 3 */
    unsigned char use_alpha; /* the base color's alpha counts, or is 1 */
    unsigned char ignore_texture_alpha;
    unsigned char use_offset; /* add the offset color */
    unsigned char punch_through; /* drop fragments of alpha below alpha_ref */
    unsigned char clamp_u, clamp_v, flip_u, flip_v;
    unsigned char filter; /* 0 point, else bilinear */
    unsigned char fog; /* the TSP's fog mode, 0 to 3 */
    /* for video_draw3's triangles, which the host turns to the screen: 2
     * drops those counterclockwise there, y down, 3 those clockwise, as the
     * ISP's culling says, and else none. video_draw's are culled already */
    unsigned char cull;
    /* the user clip: 2 draws only inside the rectangle, 3 only outside, and
     * else neither. the rectangle runs from x0, y0 to x1, y1, exclusive, in
     * the console's pixels */
    unsigned char clip_mode;
    float alpha_ref;
    unsigned short clip_x0, clip_y0, clip_x1, clip_y1;
};

/* a draw of video_draw_items: draw's triangles by count indices. where
 * vertices is null the indices name video_vertices', as video_draw's do.
 * else they name a model's vertices, vertex_count of them, in the host's
 * arena or its store, which the host turns to the screen: each one's x, y
 * and rhw fields hold its point p in the model's space, and transform is
 * three rows of four: the first two give the screen's x and y times the
 * depth, row . (p, 1), and the third the depth z, so that x and y are those
 * over z and rhw is 1 / z. every point lies in front of the eye and within
 * the drawer's guard of the screen, so the host clips nothing. rhw_near is
 * the largest rhw among them, or more, and rhw_mid one in their midst.
 * items with the same transform pointer have the same transform */
struct host_item {
    struct host_draw draw;
    const unsigned short *indices;
    int count;
    const struct host_vertex *vertices;
    int vertex_count;
    const float *transform;
    float rhw_near, rhw_mid;
    /* a model the host lights, or null: 512 ARGB words, a palette's 256
     * base colors and then its 256 offset colors, which hold until the
     * frame's video_begin. each vertex's offset field then holds its normal
     * n in the model's space, packed as the NV2A's CMP (x in bits 0 to 10,
     * y in 11 to 21, z in 22 to 31, each signed, x and y over 1023 and z
     * over 511), and its base field the base color's alpha alone. the
     * vertex takes the palette's entry (int)(d * 127.5 + 127.5), held to 0
     * to 255, where d is n's dot with light: its base color's RGB with the
     * vertex's alpha, and its offset color. so the game's drawer shades a
     * point (gcc/src/draw/model.c's shade_points), and a model the host
     * lights needn't be shaded again as it turns */
    const unsigned *palette;
    float light[3];
};

/* how the host interrupts the game for the vertical blank: where the game's
 * thread may take it, as may_interrupt says at its eip, the host keeps its
 * eip and esp in *saved_eip and *saved_esp and resumes it at entry, its
 * stack pointer stack_top, every other register as it was. a blank that
 * comes during a host call runs at the call's end, by run_now */
struct host_interrupt {
    void (*entry)(void);
    int (*may_interrupt)(unsigned long eip);
    unsigned long *saved_eip;
    unsigned long *saved_esp;
    void *stack_top;
    void (*run_now)(void);
};

struct host_api {
    unsigned version;
    /* write a line of text to the host's log */
    void (*log)(const char *text);
    /* stop the program with status */
    void (*exit)(int status);
    /* a file of the disc, by its path from the disc's root, as the game
     * names it, any case: a handle, or 0 where there is no such file. an
     * image's file loads the image's code into its code area as it opens */
    void *(*file_open)(const char *path);
    int (*file_size)(void *file);
    /* read count bytes from offset into to, and return how many it read */
    int (*file_read)(void *file, unsigned offset, void *to, int count);
    void (*file_close)(void *file);
    /* whether path names a directory of the disc */
    int (*dir_exists)(const char *path);
    /* microseconds since the host started */
    unsigned (*time_us)(void);
    /* wait for the next of the display's 60 frames a second */
    void (*wait_vblank)(void);
    /* the buttons and sticks of pad port, as port.h's PAD_ bits, or ~0 where
     * no pad is there */
    unsigned (*pad_read)(int port, int *x, int *y, int *l, int *r);
    /* interrupt the game at each vertical blank, 60 times a second, as the
     * console's interrupt preempts it, as interrupt says */
    void (*set_interrupt)(const struct host_interrupt *interrupt);
    /* the date: year, month, day, hour, minute, second, weekday from 0 */
    void (*get_date)(int fields[7]);
    /* memory cards, a file of 512-byte blocks for each drive: whether the
     * drive's file exists, and a block's read or write, 0 when it succeeds.
     * a write makes the file */
    int (*card_exists)(int drive);
    int (*card_read)(int drive, unsigned block, void *to);
    int (*card_write)(int drive, unsigned block, const void *from);
    /* video: a frame starts with the screen cleared to clear, as ARGB, takes
     * vertices (video_vertices, at the table's end) and then count / 3
     * triangles at a time as draw says, each index a vertex of the last
     * video_vertices, and shows at present. a
     * texture is width by height ARGB texels, its rows top down, which an id
     * keeps until the next video_texture of that id */
    void (*video_begin)(unsigned clear);
    void (*video_texture)(unsigned id, int width, int height, const unsigned *argb);
    void (*video_draw)(const struct host_draw *draw, const unsigned short *indices, int count);
    void (*video_present)(void);
    /* the frame's fog, before its draws: the table mode's color and the
     * vertex mode's, as ARGB, the density, and the table, 128 entries of
     * two bytes, the first in each entry's high byte */
    void (*video_fog)(unsigned table_color, unsigned vertex_color, float density, const unsigned short table[128]);
    /* the AICA's registers, the 64 KB from 0x00700000: a read or a write of
     * size bytes, 1, 2 or 4, at offset. the AICA's 2 MB of sound RAM is
     * memory the host shares with its AICA, at 0xa0800000, which the game
     * reads and writes as it is */
    unsigned (*aica_read)(unsigned offset, int size);
    void (*aica_write)(unsigned offset, unsigned value, int size);
    /* whether the host has dropped every texture video_texture gave it since
     * the last call, as a host with a fixed room for them does when it fills:
     * the renderer then sends each again as it next draws with it */
    int (*video_textures_lost)(void);
    /* the movie player's room, PORT_MOVIE_SIZE bytes, while a movie plays:
     * size bytes, or 0 to give them back. the room, or 0 where there is
     * none. a host short of memory, as the Xbox is, lends its textures' room
     * for it, which drops them (video_textures_lost) */
    void *(*movie_memory)(unsigned size);
    /* the vertices the draws that follow name by index, until the next
     * call: count of them, fewer than 65536, which stay as they are until
     * then */
    void (*video_vertices)(const struct host_vertex *vertices, int count);
    /* the texture of that key (texhash.h) from the host's pack of textures
     * converted ahead, as id: whether the host had it. a host without it,
     * or without a pack, says 0, and the renderer decodes the texture and
     * gives it by video_texture */
    int (*video_texture_packed)(unsigned id, unsigned key_lo, unsigned key_hi);
    /* whether the host's pack holds a texture of that key */
    int (*video_texture_known)(unsigned key_lo, unsigned key_hi);
    /* video memory, PORT_VRAM, is the host's to back as the port writes it:
     * memory for size bytes from offset, before their first use; 0 where
     * the host has none */
    int (*vram_commit)(unsigned offset, unsigned size);
    /* memory for the vertices of the models the port keeps from frame to
     * frame (render.c's kept models): *vertices of them, which the host's
     * graphics chip reads, and which hold as the port leaves them. the port
     * writes a model's once, in order, never reads them, and names them in
     * video_draw_items' items for as long as they stay. a host with none
     * gives a null pointer. (it has the place of video_vertices3, which
     * gave the host a model's vertices and transform, as an item does now) */
    struct host_vertex *(*video_store)(int *vertices);
    /* count draws, in their order, as host_item says: a list's draws in
     * one call, where video_draw and video_vertices3 took one each. (it
     * has the place of video_draw3, which drew video_vertices3's vertices
     * and nothing calls now) */
    void (*video_draw_items)(const struct host_item *items, int count);
    /* where the host's time went since the last call, in microseconds, for
     * the port's figures on the screen: out[0] waiting for the graphics
     * chip to finish what it was sent, out[1] waiting for the display's
     * blank to show a frame, out[2] the sound's emulation; out[3] is 0. a
     * host that doesn't measure one gives 0 */
    void (*stats)(unsigned out[4]);
    /* where the port writes the models' vertices of the frame it is making,
     * for video_draw_items' items to name: memory the host's graphics
     * chip reads, *vertices of them, written once and in order and never
     * read. it holds until the frame's video_begin, after which the call
     * gives the next frame's. a host with none gives a null pointer */
    struct host_vertex *(*video_arena)(int *vertices);
};

#define HOST (*(struct host_api *const *)HOST_API_SLOT)

/* a modern pad's stick, sx and sy from -32768 to 32767, as the console's 0
 * to 255 with 128 at rest. the console's stick rests at its center; these
 * don't, and the game moves a character at the least tilt. so a tilt within
 * dead of the center, in the stick's units, is rest, and the tilt past it
 * grows from nothing to the full throw at the rim, in the stick's direction,
 * with no step at the dead zone's edge */
#define HOST_STICK_DEAD 12 /* percent of the throw, the hosts' default */
/* pad_read's buttons: where this bit is set, bits 24 to 31 are the right
 * stick's up and down, 0 to 255, 128 at rest, down more, which the
 * console's pad hasn't (the Xbox host's; the port's camera height) */
#define HOST_PAD_RIGHT_Y (1u << 23)

static inline void host_stick(int sx, int sy, int dead, int *x, int *y)
{
    unsigned square = (unsigned)(sx * sx) + (unsigned)(sy * sy);
    unsigned tilt = 0;
    unsigned bit;
    long long past;
    int ax;
    int ay;

    /* the square's root, a bit at a time */
    for (bit = 1u << 15; bit != 0; bit >>= 1) {
        if ((tilt | bit) * (tilt | bit) <= square) {
            tilt |= bit;
        }
    }
    if (dead < 0) {
        dead = 0;
    } else if (dead > 32000) {
        dead = 32000;
    }
    if ((int)tilt <= dead) {
        *x = *y = 128;
        return;
    }
    past = (long long)(tilt - (unsigned)dead) * 128;
    ax = 128 + (int)(sx * past / ((long long)tilt * (32767 - dead)));
    ay = 128 + (int)(sy * past / ((long long)tilt * (32767 - dead)));
    *x = ax < 0 ? 0 : ax > 255 ? 255 : ax;
    *y = ay < 0 ? 0 : ay > 255 ? 255 : ay;
}

#endif
