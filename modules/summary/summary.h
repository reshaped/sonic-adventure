/* SUMMARY.PRS, which holds three screens: the story so far, how to play
 * and the staff roll. the story so far recaps the story picked on the
 * menus, up to the furthest page the player has reached. this header gives
 * SUMMARY's entry table and the story so far */
#ifndef SUMMARY_SUMMARY_H
#define SUMMARY_SUMMARY_H

#include <sg_xpt.h>

/* SUMMARY's entry table, at its first address. for each screen, the main
 * loop calls its start once, then its step on every pass until the step
 * returns other than OVERLAY_RUNNING. OVERLAY_EMPTY from the story so far's
 * start skips its step. each step has ended its screen by its last return,
 * so the main loop never calls an end. each entry ignores its argument */
struct summary {
    int (*start)(void *unused);
    int (*step)(void *unused);
    void (*end)(void *unused);
    int (*tutorial_start)(void *unused);
    int (*tutorial_step)(void *unused);
    int (*tutorial_end)(void *unused);
    int (*staff_roll_start)(void *unused);
    int (*staff_roll_step)(void *unused);
    void (*staff_roll_end)(void *unused);
};

extern struct summary summary_entries;

/* a page of the story so far: count lines, which scroll up speed dots a
 * step */
struct summary_page {
    Float speed;
    int count;
    char **lines;
};

/* a story's script: its count pages, and a flag for each. the page shown
 * is the last whose flag holds */
struct summary_script {
    struct summary_page *pages;
    Uint16 *flags;
    int count;
};

/* when a page's flag holds. with SUMMARY_PLAYED set, its low 15 bits
 * number an event, and the flag holds once that event has played.
 * otherwise it is an event flag, which holds while set. SUMMARY_NEVER
 * never holds */
#define SUMMARY_PLAYED 0x8000
#define SUMMARY_NEVER 0xffff

/* each story's script, by story number: 0 is Sonic's, and 6 the last
 * story's. the declaration leaves out the bound, 7. summary_text_step reads
 * the table through it, and with the bound in scope SHC compiles that read
 * otherwise (summary.c's @hack type) */
extern struct summary_script summary_scripts[];

/* start the story so far for the story picked on the menus. when
 * summary_find_page finds no page for it, summary_start returns
 * OVERLAY_EMPTY and changes nothing. otherwise it releases every texture,
 * resets the heap, the tasks and the streams, and starts the screen. it then
 * returns the music task as an int, which is never OVERLAY_EMPTY, and is 0
 * when task_create fails */
int summary_start(void *unused);
/* step the screen. it fades in, scrolls the page up until the page has
 * gone or until Start or A is pressed on pad 0, then fades out. it returns
 * OVERLAY_RUNNING while the screen runs, and OVERLAY_DONE once it has faded
 * out, on that step and every later one. it returns OVERLAY_RESET on the
 * pads' quit. it ends the screen itself before it returns OVERLAY_DONE or
 * OVERLAY_RESET. it never returns OVERLAY_BACK */
int summary_step(void *unused);
/* end the screen: empty the bitmap queue, destroy the streams and every
 * task, and set the texture filter back to bilinear */
void summary_end(void *unused);
/* the page of story's script to show: the last whose flag holds. returns
 * -1 when none holds, and once the story's clear flag is set. story must be
 * from 0 to 6, unchecked */
int summary_find_page(int story);

#endif
