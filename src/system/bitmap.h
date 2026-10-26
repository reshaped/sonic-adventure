/* text windows drawn into 16-bit bitmaps, and the queue that uploads them
 * as textures */
#ifndef SYSTEM_BITMAP_H
#define SYSTEM_BITMAP_H
#include <ninja.h>
#include <sg_xpt.h>

struct thread;

/* BITMAP_EVENT marks an event's bitmap, which bitmap_draw hides while
 * messages_shown is 0 */
enum bitmap_mode { BITMAP_PLAIN, BITMAP_EVENT };

/* a picture of 16-bit pixels, and the texture made from it. the texture's
 * width and height are the picture's, each rounded up to a power of two of
 * 8 or more */
struct bitmap {
    Uint8 mode; /* BITMAP_PLAIN or BITMAP_EVENT */
    Sint16 x; /* its top left on the screen */
    Sint16 y;
    Uint16 width;
    Uint16 height;
    Uint16 texture_width;
    Uint16 texture_height;
    Sint16 cursor_x; /* where the next character goes */
    Sint16 cursor_y;
    Sint16 letter_spacing; /* the dots between characters */
    Sint16 line_spacing; /* the dots between lines */
    Uint16 ink; /* the pixel text draws with */
    /* the text shows while text_color's alpha isn't 0, whatever its color */
    NJS_COLOR text_color;
    /* the band behind the text, drawn while its alpha isn't 0 */
    NJS_COLOR back_color;
    /* NULL before bitmap_alloc or bitmap_init, and after bitmap_free or
     * bitmap_release */
    Uint16 *pixels;
    Uint32 index; /* the texture's global index */
    NJS_TEXLIST texlist;
    NJS_TEXNAME texname;
};

/* 1 while events' bitmaps show, and 0 while they hide */
extern Sint8 messages_shown;

/* set bitmap up as a blank width by height picture at x, y on the screen,
 * and return it. its texture takes the global index index. its pixels come
 * from the heap, and their allocation goes unchecked */
struct bitmap *bitmap_alloc(struct bitmap *bitmap, int x, int y, int width,
                            int height, int index);
/* release bitmap's texture, and free its pixels */
void bitmap_free(struct bitmap *bitmap);
/* set bitmap up as bitmap_alloc does, but with its pixels at memory, moved
 * up to a 32-byte boundary. memory must hold the texture's pixels and up to
 * 31 bytes before them */
struct bitmap *bitmap_init(struct bitmap *bitmap, int x, int y, int width,
                           int height, int index, void *memory);
/* release bitmap's texture, and let go of its pixels, which stay the
 * caller's */
void bitmap_release(struct bitmap *bitmap);
/* set bitmap up as the message window, with its pixels from the heap or
 * from the window's own memory */
void bitmap_alloc_window(struct bitmap *bitmap);
void bitmap_init_window(struct bitmap *bitmap);

/* blank bitmap's pixels, and move its cursor to the top left */
void bitmap_clear(struct bitmap *bitmap);
/* move bitmap's cursor to the start of the next line */
void bitmap_newline(struct bitmap *bitmap);
/* draw Shift JIS text into bitmap's pixels at the cursor, starting a new
 * line at each newline. text that starts with a tab centers each line on
 * its own, and text that starts with a bell centers the lines as a block.
 * its texture shows the text after bitmap_queue and the next upload */
void bitmap_print(struct bitmap *bitmap, const char *text);
/* format into a 128-byte buffer, whose bounds go unchecked, and
 * bitmap_print the result */
void bitmap_printf(struct bitmap *bitmap, const char *format, ...);

/* the same as bitmap_queue_add, which it calls */
void bitmap_queue(struct bitmap *bitmap);
/* queue bitmap for bitmap_flush_queue to upload as its texture. the queue
 * holds 8, and drops any past the eighth. bitmap and its pixels must stay
 * valid until the flush */
void bitmap_queue_add(struct bitmap *bitmap);
/* empty the queue, uploading nothing */
void bitmap_clear_queue(void);
/* empty the queue, uploading a bitmap as its texture for each entry. the
 * bitmaps uploaded are the first one queued and those after it in memory,
 * not the queue's later entries. each upload releases any texture the
 * bitmap had. frame_wait calls it */
void bitmap_flush_queue(void);
/* draw back_color's band across the screen, from 4 dots above the bitmap
 * to 4 below it. then draw the texture over it, in a fixed pale blue. each
 * draws only while its color's alpha isn't 0, and the texture only once
 * uploaded. an event's bitmap draws nothing while messages_shown is 0 */
void bitmap_draw(struct bitmap *bitmap);

/* fade bitmap in, drawing it and yielding the calling thread at each step.
 * its band fades in to half alpha over 10 steps, then its text to full
 * alpha over 10 more */
void bitmap_fade_in(struct bitmap *bitmap);
/* fade bitmap out, the reverse of bitmap_fade_in, then draw it 2 steps
 * more */
void bitmap_fade_out(struct bitmap *bitmap);
/* fade the text alone in or out over 8 steps. bitmap_fade_out_text then
 * draws it 2 steps more */
void bitmap_fade_in_text(struct bitmap *bitmap);
void bitmap_fade_out_text(struct bitmap *bitmap);
/* draw bitmap and yield, steps times */
void bitmap_wait(struct bitmap *bitmap, int steps);

/* start a child thread with a window that offers text's lines, and return
 * it. up and down move the bar, and A or C chooses. the thread's status is
 * 0 until the choice, and then the line chosen, counted from 1. the thread
 * then waits for its owner to end it. text's lines count up to its last
 * nonempty one */
struct thread *bitmap_choose(const char *text);
/* the same, with count lines given as an array */
struct thread *bitmap_choose_lines(int count, const char **lines);

/* show pages, a NULL-ended array of texts, in the message window a page at
 * a time. a thread of its own shows them, and the call returns without
 * waiting. a page shows for 120 steps, or until b is pressed. pages and its
 * texts must stay valid until the thread ends */
void bitmap_show_pages(char **pages);
/* the same, but each page shows until b is pressed */
void bitmap_show_pages_until_b(char **pages);
/* TRUE from bitmap_show_pages or bitmap_show_pages_until_b until its
 * thread ends, and FALSE otherwise */
Bool bitmap_pages_active(void);

#endif
