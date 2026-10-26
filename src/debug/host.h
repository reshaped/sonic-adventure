/* requests between the game and the development host, which reads and
 * writes the game's memory while it runs */
#ifndef DEBUG_HOST_H
#define DEBUG_HOST_H
#include <sg_xpt.h>

struct task;

/* replace the task's step with one that takes a snapshot of the camera and
 * the players for the host */
void host_snapshot_each_step(struct task *task);
/* serve the host's request, if one is pending. it runs as each render
 * ends */
void host_serve_request(void);
/* clear the host's request and the game's, leaving neither pending */
void host_init(void);
/* host_load_file asks the development host to load the file named name
 * into dest, and host_save_file to save size bytes of src as name. each
 * waits, a vsync at a time and with no timeout, until the host has served
 * the request. dest must hold the whole file. the game's requests share one
 * record, so a call must not interrupt another. host_load_file returns the
 * host's answer, which its callers take as the file's size, or below 0 for
 * a failed load */
Sint32 host_load_file(const char *name, void *dest);
void host_save_file(const char *name, void *src, int size);
/* make the game's request 3 of the host with three words, and wait as
 * host_load_file does */
void fun_8c010a9e(Sint32 word0, Sint32 word1, Sint32 word2);
/* make the game's request 4 of the host with one word, wait as
 * host_load_file does, and return the host's answer */
Sint32 fun_8c010ae4(Sint32 word);

#endif
