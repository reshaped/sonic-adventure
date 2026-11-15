/* the Xbox host's video, video.c: host_api's video calls on the NV2A */
#ifndef XBOX_HOST_XBOX_VIDEO_H
#define XBOX_HOST_XBOX_VIDEO_H

#include "../../port/host.h"

int video_open(void);
void video_begin(unsigned clear);
void video_texture(unsigned id, int width, int height, const unsigned *argb);
void video_vertices(const struct host_vertex *vertices, int count);
int video_texture_packed(unsigned id, unsigned key_lo, unsigned key_hi);
int video_texture_known(unsigned key_lo, unsigned key_hi);
void video_vertices3(const struct host_vertex *vertices, int count, const float transform[12], float rhw_near,
                     float rhw_mid);
struct host_vertex *video_store(int *vertices);
void video_draw3(const struct host_draw *draw, const unsigned short *indices, int count);
void video_draw_items(const struct host_item *items, int count);
struct host_vertex *video_arena(int *vertices);
void video_idle_present(void);
extern int video_bloom_on;
int video_hd(void);
void video_pool_kb(unsigned *used, unsigned *size);
int video_blank_locked(void);
unsigned video_blank_count(void);
int video_blank_flagged(void);
void video_draw(const struct host_draw *draw, const unsigned short *indices, int count);
void video_present(void);
void video_fog(unsigned table_color, unsigned vertex_color, float density, const unsigned short table[128]);
int video_textures_lost(void);
void *video_movie_memory(unsigned size);
void video_report(void);
void video_report_memory(void);
void video_shot_burst(void);
int video_give_memory(unsigned bytes);
void video_test(void);

#endif
