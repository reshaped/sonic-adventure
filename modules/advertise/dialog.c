/* text dialogs: windows of lines drawn into a bitmap. a dialog grows open,
 * shows until closed, and shrinks away */
#include <ninja.h>
#include <sg_xpt.h>
#include <string.h>

#include "advertise/dialog.h"
#include "draw/screen.h"
#include "system/bitmap.h"
#include "system/task.h"

#define TEXT_1_LENGTH 28
#define TEXT_2_LENGTH 20
#define TEXT_3_LENGTH 16
#define TEXT_4_LENGTH 28
#include "advertise/dialog/text.inc"

/* a line's characters take CHARACTER_BYTES bytes each, in Shift JIS. a
 * character is CHARACTER_WIDTH dots wide, and a line LINE_HEIGHT dots high */
#define CHARACTER_BYTES 2
#define CHARACTER_WIDTH 26
#define LINE_HEIGHT 30

/* how far a dialog opens or closes each step, of the way from 0 to 1 */
#define DIALOG_STEP 0.2F

struct dialog_work {
    Uint8 mode;
    Float open; /* how far open, from 0 to 1 */
    Float step;
    struct dialog_spec spec;
    struct bitmap bitmap;
    NJS_TEXTURE_VTX vertices[4];
};

static char *dialog_sample_lines[] = {
    TEXT_1, "", TEXT_2, TEXT_3, TEXT_4,
};
struct dialog_spec dialog_sample = {
    dialog_sample_lines, 5, 1, 320.0F, 240.0F, 10.0F, -1, -16,
};

/* the pixels of every dialog's bitmap. the array holds a 512 by 512
 * texture's, at 2 bytes each. it has 32 bytes more for the 31 at most that
 * bitmap_init skips to reach a 32-byte boundary */
static char dialog_pixels[(512 * 512 * 2) + 32];

static void dialog_place(struct dialog_work *work)
{
    Float x1;
    Float x2;
    Float y1;
    Float y2;
    Float z;
    Float u1;
    Float v1;
    Float u0;
    Float v0;
    int centered = work->spec.centered;

    z = 1.0F / work->spec.z;
    if (centered == 1) {
        /* width / 2 and height / 2 below drop the odd dot */
        /* NOLINTBEGIN(bugprone-integer-division) */
        x1 = work->bitmap.x - ((work->bitmap.width / 2) * work->open);
        y1 = work->bitmap.y - ((work->bitmap.height / 2) * work->open);
        x2 = work->bitmap.x + ((work->bitmap.width / 2) * work->open);
        y2 = work->bitmap.y + ((work->bitmap.height / 2) * work->open);
        /* NOLINTEND(bugprone-integer-division) */
    } else {
        x1 = work->bitmap.x;
        y1 = work->bitmap.y;
        x2 = work->bitmap.x + (work->bitmap.width * work->open);
        y2 = work->bitmap.y + (work->bitmap.height * work->open);
    }
    u0 = v0 = 0.0F;
    u1 = (Float)work->bitmap.width / work->bitmap.texture_width;
    v1 = (Float)work->bitmap.height / work->bitmap.texture_height;
    work->vertices[0].x = x1;
    work->vertices[0].y = y1;
    work->vertices[0].z = z;
    work->vertices[0].u = u0;
    work->vertices[0].v = v0;
    work->vertices[1].x = x1;
    work->vertices[1].y = y2;
    work->vertices[1].z = z;
    work->vertices[1].u = u0;
    work->vertices[1].v = v1;
    work->vertices[2].x = x2;
    work->vertices[2].y = y1;
    work->vertices[2].z = z;
    work->vertices[2].u = u1;
    work->vertices[2].v = v0;
    work->vertices[3].x = x2;
    work->vertices[3].y = y2;
    work->vertices[3].z = z;
    work->vertices[3].u = u1;
    work->vertices[3].v = v1;
}

static void dialog_step(struct task *task)
{
    struct dialog_work *work = task->extra;
    NJS_TEXTURE_VTX *vertices;

    switch (work->mode) {
    case DIALOG_OPEN:
        work->open += work->step;
        if (work->open >= 1.0F) {
            work->open = 1.0F;
            work->mode = DIALOG_SHOWN;
        }
        break;
    case DIALOG_SHOWN:
        break;
    case DIALOG_CLOSE:
        work->open -= work->step;
        if (work->open <= 0.0F) {
            work->open = 0.0F;
            work->mode = DIALOG_DONE;
        }
        break;
    case DIALOG_DONE:
        task_kill(task);
        break;
    }
    if (work->open > 0.0F) {
        dialog_place(work);
        njDrawTexture(work->vertices, 4, work->spec.index, 1);
        screen_quad_colors(0xc0002e67, 0xc0117bff, 0xc0002e67, 0xc0117bff);
        vertices = work->vertices;
        screen_box_draw(vertices[0].x - 16.0F, vertices[0].y - 16.0F,
                        work->spec.z + 10.0F,
                        (vertices[2].x - vertices[0].x) + 32.0F,
                        (vertices[1].y - vertices[0].y) + 32.0F);
    }
}

static void dialog_destroy(struct task *task)
{
    struct dialog_work *work = task->extra;

    if (work->bitmap.pixels) {
        bitmap_release(&work->bitmap);
    }
}

struct task *dialog_create(int level, const struct dialog_spec *spec)
{
    struct task *task = task_create(0, level, dialog_step);
    struct dialog_work *work;
    int longest;
    int i;
    int width;
    int height;

    if (!task) {
        return task;
    }
    work = heap_calloc(1, sizeof(struct dialog_work));
    task->extra = work;
    work->spec = *spec;
    /* @hack spelling: i set ahead of longest, and outside the loop's head. SHC
     * then clears i, in R12, before the spec's copy (0x8c90b716 in
     * ADVERTISE), and longest, in R11, in the slot of the BF/S at 0x8c90b73c.
     * as for (i = 0; ...) after longest = 0, the two clears trade places, and
     * 2 of 143 words differ */
    i = 0;
    longest = 0;
    for (; i < work->spec.count; i++) {
        if ((int)strlen(work->spec.lines[i]) > longest) {
            longest = strlen(work->spec.lines[i]);
        }
    }
    width = longest * CHARACTER_WIDTH / CHARACTER_BYTES;
    height = work->spec.count * LINE_HEIGHT;
    bitmap_init(&work->bitmap, (int)work->spec.x, (int)work->spec.y, width,
                height, work->spec.index, dialog_pixels);
    for (i = 0; i < work->spec.count; i++) {
        bitmap_print(&work->bitmap, work->spec.lines[i]);
        bitmap_newline(&work->bitmap);
    }
    bitmap_queue(&work->bitmap);
    work->vertices[0].col = work->vertices[1].col = work->vertices[2].col =
        work->vertices[3].col = 0xffffffff;
    work->step = DIALOG_STEP;
    task->destructor = dialog_destroy;
    return task;
}

enum dialog_mode dialog_mode(struct task *task)
{
    struct dialog_work *work = task->extra;

    return work->mode;
}

Bool dialog_close(struct task *task)
{
    struct dialog_work *work = task->extra;

    if (work->mode == DIALOG_SHOWN) {
        work->mode = DIALOG_CLOSE;
        return TRUE;
    }
    return FALSE;
}

int dialog_pixels_size(int width, int height)
{
    int texture_width = 8;
    int texture_height;

    while (texture_width < width) {
        texture_width <<= 1;
    }
    texture_height = 8;
    while (texture_height < height) {
        texture_height <<= 1;
    }
    /* @hack spelling: the product held in width before the add. SHC then
     * moves it from R4 to R0 to add 32 (MOV R4,R0 at 0x8c90b87e in
     * ADVERTISE). returned directly, the product goes straight to R0, the
     * function runs 2 bytes short, and 4 of 18 words differ */
    width = texture_width * texture_height;
    return width + 32;
}
