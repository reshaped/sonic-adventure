/* curves: records sampled round a closed curve through keys, and runs along
 * one axis */
#include <ninja.h>
#include <sg_xpt.h>

#include "path/curve.h"

#define FIELDS_MAX 19

/* lay out format's fields: offsets gets each field's offset, then the
 * record's size. returns the count of fields */
static int layout_fields(const char *format, int *offsets)
{
    int offset = 0;
    int i = 0;
    int align = 1;
    int pad;

    for (; *format != '\0'; format++, i++) {
        switch (*format) {
        case 'C':
        case 'c':
            offsets[i] = offset;
            offset++;
            break;
        case 'S':
        case 's':
            offset += offset % 2;
            offsets[i] = offset;
            offset += 2;
            if (align < 2) {
                align = 2;
            }
            break;
        case 'A':
        case 'F':
        case 'I':
        case 'L':
        case 'a':
        case 'f':
        case 'i':
        case 'l':
            pad = offset % 4;
            if (pad != 0) {
                offset += 4 - pad;
            }
            offsets[i] = offset;
            offset += 4;
            if (align < 4) {
                align = 4;
            }
            break;
        default:
            *(int *)1 = 0;
            break;
        }
    }
    if (offset % align != 0) {
        offset = ((offset / align) + 1) * align;
    }
    offsets[i] = offset;
    return i;
}

/* the field at offsets[field] of key n */
#define KEY_FIELD(type, n)                                                     \
    (*(type *)((char *)curve->keys + ((n) * size) + offsets[field]))

/* set out's field to the sum of keys a to d's, each times its weight */
#define BLEND(type, a, b, c, d)                                                \
    (*(type *)(out + offsets[field]) =                                         \
         (weight[0] * KEY_FIELD(type, a)) + (weight[1] * KEY_FIELD(type, b)) + \
         (weight[2] * KEY_FIELD(type, c)) + (weight[3] * KEY_FIELD(type, d)))

/* set out's field from the four keys round key, key - 1 to key + 2, which
 * wrap round the loop */
#define SAMPLE(type)                                                           \
    do {                                                                       \
        if (key == 0) {                                                        \
            BLEND(type, curve->count - 1, 0, 1, 2);                            \
        } else if (key == curve->count - 2) {                                  \
            BLEND(type, key - 1, key, key + 1, 0);                             \
        } else if (key == curve->count - 1) {                                  \
            BLEND(type, key - 1, key, 0, 1);                                   \
        } else {                                                               \
            BLEND(type, key - 1, key, key + 1, key + 2);                       \
        }                                                                      \
    } while (0)

int path_curve_sample(struct path_curve *curve)
{
    int offsets[FIELDS_MAX + 1]; /* one per field, then the record's size */
    int size;
    int key;
    int field;
    Float weight[4];
    Float part;
    /* @hack redundant: records zeroed here and again before the loop, the
     * image's two stores (0x8c09b002, 0x8c09b01e); without it, the function
     * runs 32 bytes short and 4,293 of 4,417 words differ */
    int records = 0;
    int fields;
    const char *format;
    char *out;
    Float part_to[4]; /* part to the power of the index, 2 or 3 */
    Float rest_to[4]; /* 1 - part to the power of the index, 1 to 3 */

    fields = layout_fields(curve->format, offsets);
    size = offsets[fields];
    records = 0;
    for (key = 0; key < curve->count; key++) {
        /* NOLINTNEXTLINE(bugprone-float-loop-counter) */
        for (part = 0.0F; part <= 1.0F - curve->step; part += curve->step) {
            switch (curve->kind) {
            case PATH_CURVE_LAGRANGE:
                weight[0] = -part * (part - 1.0F) * (part - 2.0F) / 6.0F;
                weight[1] =
                    (part + 1.0F) * (part - 1.0F) * (part - 2.0F) / 2.0F;
                weight[2] = -(part + 1.0F) * part * (part - 2.0F) / 2.0F;
                weight[3] = (part + 1.0F) * part * (part - 1.0F) / 6.0F;
                break;
            case PATH_CURVE_BSPLINE:
            default:
                part_to[2] = part * part;
                part_to[3] = part_to[2] * part;
                rest_to[1] = 1.0F - part;
                rest_to[2] = (1.0F - part) * (1.0F - part);
                rest_to[3] = rest_to[1] * rest_to[2];
                /* weight[0] and weight[3] multiply by 1/6, 0x3e2aaaaa, as SHC's
                 * code under -aggressive=2 does */
                weight[0] = rest_to[3] * (1.0F / 6.0F);
                weight[1] = (part_to[3] / 2.0F) - part_to[2] + (2.0F / 3.0F);
                weight[2] = (-part_to[3] / 2.0F) + (part_to[2] / 2.0F) +
                            (part / 2.0F) + (1.0F / 6.0F);
                weight[3] = part_to[3] * (1.0F / 6.0F);
                break;
            }
            format = curve->format;
            out = (char *)curve->out + (size * records);
            for (field = 0; field < fields; format++, field++) {
                switch (*format) {
                case 'c':
                    SAMPLE(Uint8);
                    break;
                case 'C':
                    SAMPLE(Sint8);
                    break;
                case 's':
                    SAMPLE(Uint16);
                    break;
                case 'S':
                    SAMPLE(Sint16);
                    break;
                case 'l':
                    SAMPLE(Uint32);
                    break;
                case 'A':
                case 'L':
                case 'a':
                    SAMPLE(Sint32);
                    break;
                case 'I':
                case 'i':
                    SAMPLE(int);
                    break;
                case 'F':
                case 'f':
                    SAMPLE(Float);
                    break;
                }
            }
            records++;
        }
    }
    return records;
}

Float path_axis_position(const Float *stops, Float distance)
{
    /* @hack type: i a long, not an int, which has SHC add 1 to i before it
     * scales it to index stops[i + 1] (ADD #1,R0 at 0x8c09d94c); as an int,
     * 38 of 52 words differ */
    Sint32 i;
    Float length = 0.0F;
    Float before;

    for (i = 0; length <= distance; i++) {
        if (stops[i + 1] < 0.0F) {
            return stops[i];
        }
        before = length;
        length += njAbs(stops[i + 1] - stops[i]);
    }
    i--;
    if (stops[i + 1] - stops[i] > 0.0F) {
        return stops[i] + distance - before;
    }
    return stops[i] - distance + before;
}

Float path_axis_length(const Float *stops)
{
    /* @hack type: i a long, not an int, as in path_axis_position (ADD #1,R0
     * at 0x8c09d9ca); as an int, the function runs 2 bytes long and 25 of 29
     * words differ */
    Sint32 i;
    Float length = 0.0F;

    for (i = 0; stops[i + 1] >= 0.0F; i++) {
        length += njAbs(stops[i + 1] - stops[i]);
    }
    return length;
}
