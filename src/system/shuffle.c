/* shuffles: an array's records in a random order */
#include <sg_xpt.h>
#include <stdlib.h>

#include "system/shuffle.h"
#include "system/util.h"

static void memory_swap(void *one, void *other, int size)
{
    int i = 0;
    Uint8 *left = one;
    Uint8 *right = other;
    Uint8 byte;

    while (i < size) {
        byte = *left;
        *left = *right;
        *right = byte;
        left++;
        right++;
        i++;
    }
}

void array_shuffle(void *array, int count, int size)
{
    Uint8 *base = array;
    Uint8 *last = base + (count * size);
    int left = count;
    int pick;

    while (left > 1) {
        pick = (int)((Float)rand() * (Float)left * RAND_SCALE);
        left--;
        last -= size;
        if (pick != left) {
            memory_swap(base + (pick * size), last, size);
        }
    }
}
