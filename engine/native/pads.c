/* the first pad's record, which play's live pad or a script of keys fills at
 * each scan. the link's --wrap sends the game's calls of pad_scan to
 * __wrap_pad_scan. its hooks (native.h) may stand in for the keys, and run
 * either side of the game's scan */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "native.h"

void __real_pad_scan(void);

int native_scripted;
static FILE *keys;
static struct native_pad *pad_record;

void native_pad_record(void *pad)
{
    pad_record = pad;
}

/* open the script of keys at path. each line holds a vsync in decimal and the
 * buttons held from it, Shinobi's PDD_DGT bits in hex. a line may go on
 * with the stick's x and y, from -128 to 127. both are 0 where it doesn't. a
 * line of any other form ends the script, and its last pad stays held */
void native_keys_open(const char *path)
{
    if (!(keys = fopen(path, "r"))) {
        fprintf(stderr, "sa1: no keys at %s\n", path);
        exit(2);
    }
    native_scripted = 1;
}

static void next_keys(void)
{
    static unsigned long at;
    static uint32_t held, next;
    static int x, y, next_x, next_y;
    uint32_t last = pad_record->on;
    char line[128];

    while (keys && native_vsyncs >= at) {
        held = next;
        x = next_x;
        y = next_y;
        next_x = next_y = 0;
        if (!fgets(line, sizeof line, keys) ||
            sscanf(line, "%lu %x %d %d", &at, &next, &next_x, &next_y) < 2) {
            fclose(keys);
            keys = NULL;
        }
    }
    pad_record->x1 = (int16_t)x;
    pad_record->y1 = (int16_t)y;
    pad_record->on = held;
    pad_record->off = ~held;
    pad_record->press = held & ~last;
    pad_record->release = last & ~held;
}

void (*native_pad_read)(void *pad);

void __wrap_pad_scan(void)
{
    if (native_pad_read && pad_record) {
        native_pad_read(pad_record);
        sa1_chord(pad_record);
    } else if (pad_record && !native_hook_pads()) {
        next_keys();
        sa1_chord(pad_record);
    }
    native_hook_pad_scan(pad_record);
    __real_pad_scan();
    native_hook_pad_scanned();
}
