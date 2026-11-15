/* the PC host's video and keyboard, in video.c */
#ifndef XBOX_HOST_PC_VIDEO_H
#define XBOX_HOST_PC_VIDEO_H

struct host_draw;
struct host_vertex;

int video_open(void);
void video_begin(unsigned clear);
void video_texture(unsigned id, int width, int height, const unsigned *argb);
void video_vertices(const struct host_vertex *vertices, int count);
int video_texture_known(unsigned key_lo, unsigned key_hi);
void video_vertices3(const struct host_vertex *vertices, int count, const float transform[12], float rhw_near,
                     float rhw_mid);
struct host_vertex *video_store(int *vertices);
void video_draw3(const struct host_draw *draw, const unsigned short *indices, int count);
void video_draw_items(const struct host_item *items, int count);
int video_texture_packed(unsigned id, unsigned key_lo, unsigned key_hi, int have_video);
void video_draw(const struct host_draw *draw, const unsigned short *indices, int count);
void video_present(void);
void video_fog(unsigned table_color, unsigned vertex_color, float density, const unsigned short table[128]);
unsigned video_keys(void);
/* the keyboard's stick and triggers, where a key moves them */
void video_stick(int *x, int *y, int *l, int *r);
int video_quit(void);
int video_shot(const char *path);

#endif
