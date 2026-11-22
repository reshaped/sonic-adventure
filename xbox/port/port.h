/* the port layer's own calls, which its sources share */
#ifndef XBOX_PORT_PORT_H
#define XBOX_PORT_PORT_H

/* the port layer's memory, past the console's 16 MB of RAM in area 3, which
 * the host maps with RAM, and at the store queues as RAM is: Kamui's lists
 * live here. its 18.5 MB, and 5 MB more for a movie, with RAM's 16 MB fit
 * the Xbox's 64 MB with the code areas, sound RAM, the host and its
 * textures: the lists are sized by their use, and the renderer's tables at
 * three times and more the most a frame has needed (render.c) */
#define PORT_MEMORY 0x0d000000u
/* in it: Kamui's five lists, then video memory, then the renderer's room,
 * then the movie player's, which the PC host maps with the rest and the
 * Xbox host doesn't: the player takes its room through host_api's
 * movie_memory as a movie starts. the lists by ListType: opaque, its modifier
 * volumes, translucent, its modifiers, punch-through. the game draws no
 * modifier volumes. since the models and sprites go to the host as runs,
 * the lists hold their headers and the runs' marks: the most a frame put in
 * them in Speed Highway's third act, on the console, is 82 KB opaque, 44 KB
 * translucent and none punch-through. 512 KB in all, the opaque 256, the
 * translucent 192 and the punch-through 32. a list past its room runs into
 * the next, and the last into video memory: kmRender logs it */
#define PORT_LIST_SIZES {0x00040000u, 0x00004000u, 0x00030000u, 0x00004000u, 0x00008000u}
#define PORT_LISTS_SIZE 0x00080000u
#define PORT_LISTS PORT_MEMORY
#define PORT_VRAM (PORT_MEMORY + PORT_LISTS_SIZE)
#define PORT_VRAM_SIZE 0x00800000u
#define PORT_VRAM_PAGE 0x10000u
#define PORT_RENDER (PORT_VRAM + PORT_VRAM_SIZE)
#define PORT_RENDER_SIZE 0x00a00000u
/* what of it the renderer's tables use, and the Xbox host maps: the rest is
 * address space only. 3.5 MB: the tables take 3,511 KB (render.c) */
#define PORT_RENDER_USED 0x00380000u
#define PORT_MOVIE (PORT_RENDER + PORT_RENDER_SIZE)
#define PORT_MOVIE_SIZE 0x00500000u
#define PORT_MEMORY_SIZE (PORT_MOVIE + PORT_MOVIE_SIZE - PORT_MEMORY)

void port_log(const char *text);
void port_unimplemented(const char *name);

/* video memory's backing: before a read or a write of size bytes at offset.
 * 0 where the host has no memory for them */
int port_vram_commit(unsigned offset, unsigned size);

/* a texture kmLoadTexture gave the host's pack by its key, in place of its
 * bytes in video memory (xbox/port/kamui.c): the one at base of that
 * format, kind and sizes, its key and the serial of its load */
struct texhash;
int port_texture_at(unsigned base, unsigned format, int vq, int raster, unsigned width, unsigned height,
                    struct texhash *key, unsigned *serial);

/* the profile's counts, which the host turns on (port_profile): where a
 * frame's vertices come from, and the game's models whose points changed
 * since they were last drawn. the renderer logs them */
struct port_counts {
    unsigned drawer_models, drawer_points, drawer_vertices; /* src/draw's model drawer */
    unsigned changed_models, changed_points;
    unsigned ninja_vertices; /* njModel.c's drawer */
    unsigned packed_loads, copied_loads; /* kmLoadTexture's */
    unsigned gpu_vertices; /* the drawer's that the host transforms */
    unsigned kept_vertices, built_vertices; /* those of them from the kept models' store, and built into it */
    /* the timestamp counter's counts in the drawer's entries and in the
     * renderer, its calls of the host with it */
    unsigned long long drawer_tsc, render_tsc;
};
extern struct port_counts port_counts;
/* bit 0: the counts and the times; bit 1: the changed models too, a sum
 * over each model's points as it's drawn */
extern volatile int port_profile;
/* the port's figures on the screen, a second's or so, in Ninja's debug
 * text (port_overlay_draw, which Kamui's kmRender calls): the host sets it */
extern volatile int port_overlay;
struct port_shown {
    unsigned fps10; /* frames a second, in tenths */
    unsigned frame10, drawer10, render10; /* a frame's milliseconds, in tenths: all of it, the game's drawer, the renderer */
    unsigned vertices, gpu_vertices, models;
    unsigned gpu_wait10, blank_wait10, sound10; /* a frame's milliseconds, in tenths: host_api's stats */
    unsigned pack_kb; /* video memory backed, in KB */
    unsigned worst10; /* the longest of the 60 frames, in tenths of a millisecond: an even 30 a second is 33.4 */
};
extern struct port_shown port_shown;
void port_overlay_draw(void);
/* the game's drawer sends the models it can to the host untransformed
 * (gcc/src/draw/model.c): bit 0 their opaque meshsets, bit 1 their
 * translucent ones too; 0 turns it off */
extern volatile int port_gpu_models;
/* bit 2 of it: a model that is as it was when last drawn keeps its vertices
 * in the host's store (render.c's kept models), and isn't shaded or written
 * again. bit 3: a model whose sphere fails the drawer's test for the host
 * is tried by its box */
void port_count_model(const void *model, const void *points, int count);
/* each model the drawer draws, for the watch (report.c) */
void port_watch_model(const void *model);

/* the report (report.c). each frame's phases, in microseconds, and its
 * counts, kept for the last PORT_FRAMES frames in port_frames, the newest
 * at (port_frame_count - 1) % PORT_FRAMES. the host reads them by symbol
 * after a fault or a stall */
#define PORT_FRAMES 128
enum port_phase {
    PHASE_FRAME, /* from one render to the next */
    PHASE_BUSY, /* the game's frame: its logic and its drawer, without its wait for the blank */
    PHASE_LOGIC, /* the busy time less the drawer */
    PHASE_DRAWER, /* src/draw's model drawer */
    PHASE_RENDER, /* port_render, its host calls among it */
    PHASE_LISTS, /* of it, reading Kamui's lists */
    PHASE_DRAWS, /* of it, handing the strips and runs to the host, the sort among it */
    PHASE_SORT, /* of the draws, the translucent list's sort */
    PHASE_GPU_WAIT, /* the host's wait for the graphics chip */
    PHASE_BLANK_WAIT, /* the host's wait for the blank to show a frame */
    PHASE_SOUND, /* the host's sound, at the blanks of the frame */
    PHASE_IDLE, /* what is left: the frame's wait for its blank */
    PHASES
};
struct port_frame {
    unsigned number; /* the renderer's frame */
    unsigned short us[PHASES]; /* 65535 at most */
    unsigned short strips, models;
    unsigned vertices, gpu_vertices;
    unsigned char steps; /* the game's logic steps the frame showed (frame_step): 2 at 30 a second, 3 for an extra step */
    unsigned char steps_meant; /* frame_step_count, the steps it means a frame to take */
    unsigned short pad;
};
extern struct port_frame port_frames[PORT_FRAMES];
extern volatile unsigned port_frame_count;
/* where the game is, as of the last frame: for the report, and for the host
 * after a fault. pos and ang are player 0's and the camera's work's */
struct port_where {
    int game_mode;
    int stage, act, character;
    int player; /* 1 where player 0 has a work */
    float player_pos[3];
    int player_ang[3];
    int camera; /* 1 where the camera has a work */
    float camera_pos[3];
    int camera_ang[3];
};
extern struct port_where port_where;
/* set by the host for a report: the port logs its part at the next render
 * and clears it */
extern volatile int port_report;
/* a place to put player 0 at as it enters a stage and act, as a restart
 * point does (stage_place_player, under XBOX), taken once. the report gives
 * the HOST_POKE that sets it: armed, then the stage, the act, the place's
 * x, y and z as their floats' bits, the angles, the character, and enter:
 * set, the menus' first step enters the stage and act as the trial menu
 * does, as that character (port_place_enter), so a report's place is
 * reached without a pad's script */
struct port_place {
    int armed;
    int stage, act;
    float pos[3];
    int ang[3];
    int character;
    int enter;
};
/* src/main.c's, under XBOX, at the menus' step: TRUE once where port_place
 * asks for its stage to be entered, with the stage, act and character set */
int port_place_enter(void);
extern volatile struct port_place port_place;
/* the frame's end: render.c's call at the end of port_render, with the
 * renderer's time and its parts in timestamp counts */
void port_report_frame(unsigned long long render_tsc, unsigned long long lists_tsc, unsigned long long draws_tsc,
                       unsigned long long sort_tsc, int strips, int vertices);
/* render.c's: after it clears port_counts, every 60 frames */
void port_report_counts_reset(void);
/* the host's times (host_api's stats) since the last call: the report asks
 * the host each frame and the overlay takes them from it */
void port_report_host_us(unsigned out[4]);
/* render.c's lines of the report: its tables' use */
void port_render_tables(void);
/* gcc/src/system/pace.c's call, under XBOX: the game's busy time of the
 * frame ending, in microseconds */
void port_frame_busy(unsigned us);

#endif
