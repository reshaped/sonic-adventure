/* decoding PRS, the LZ77 that packs the disc's files. make pc compiles this
 * in place of src/system/prs.src, whose comments give the format.
 *
 * the decode reads its stream in prs.src's order: a control byte only when a
 * bit is needed and the last byte's eight are spent, so it lies between the
 * stream's other bytes where the encoder put it. a copy reads its source a
 * byte at a time, so a copy that overlaps its own output repeats it, as
 * prs.src's does */
#include <sg_xpt.h>

#include "system/prs.h"

/* the stream's position, and its control bits left and their number */
struct prs_stream {
    const Uint8 *in;
    Uint32 bits;
    Sint32 left;
};

/* the next control bit, low bit first */
static Uint32 prs_bit(struct prs_stream *stream)
{
    Uint32 bit;

    if (stream->left == 0) {
        stream->bits = *stream->in++;
        stream->left = 8;
    }
    bit = stream->bits & 1;
    stream->bits >>= 1;
    stream->left--;
    return bit;
}

Sint32 prs_decompress(void *from, void *to)
{
    struct prs_stream stream;
    Uint8 *out = to;

    stream.in = from;
    stream.bits = *stream.in++;
    stream.left = 8;
    for (;;) {
        Sint32 back;
        Sint32 length;

        if (prs_bit(&stream)) {
            *out++ = *stream.in++;
            continue;
        }
        if (prs_bit(&stream)) {
            /* a long copy's word, low byte first: 13 bits of distance over 3
             * of length. a word of 0 ends the stream */
            Uint32 word = stream.in[0] | (Uint32)stream.in[1] << 8;

            stream.in += 2;
            if (word == 0) {
                break;
            }
            back = (Sint32)(word >> 3) - 8192;
            length = word & 7;
            if (length != 0) {
                length += 2;
            } else {
                /* a byte n after the word gives n + 1 bytes */
                length = *stream.in++ + 1;
            }
        } else {
            /* a short copy: two bits of length, high first, then a byte of
             * distance, from 1 to 256 back */
            length = prs_bit(&stream) << 1;
            length |= prs_bit(&stream);
            length += 2;
            back = (Sint32)*stream.in++ - 256;
        }
        {
            const Uint8 *source = out + back;

            do {
                *out++ = *source++;
            } while (--length != 0);
        }
    }
    return (Sint32)(out - (Uint8 *)to);
}
