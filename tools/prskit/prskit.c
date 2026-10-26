// PRS compression and decompression
//
// PRS is LZ77 with control bits interleaved with data. the decoder reads
// a control byte whenever it needs a bit and has none left, and takes the
// bits from the low end. each item is one of:
//
//   1 b        the byte b
//   00 x y b   copy 2x+y+2 bytes (2-5) from 256-b back (1-256)
//   01 w       copy (w&7)+2 bytes (3-9) from 8192-(w>>3) back (1-8192)
//   01 w n     the same, with w&7 0: copy n+1 bytes (1-256), from 1-8191 back
//   01 0       the end, which is why the line above stops at 8191
//
// 1, 00, 01, x and y are control bits, b and n are bytes, and w is a
// little-endian word. a copy can overlap the bytes it writes, so a run of one
// byte is a copy from one byte back. the last control byte's spare bits are
// 0, and nothing follows the end
#include "prskit.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// the format permits an 8192-byte back-reference; the encoder uses an
// 8191-byte window and an 8192-slot chain ring
#define WINDOW 8191
#define MAX_LENGTH 256
#define HASH_BITS 15

// decoder input position and remaining control bits
struct reader {
    const unsigned char *src;
    size_t len;
    size_t pos;
    size_t control_at;
    unsigned control;
    unsigned left;
};

static unsigned next_bit(struct reader *in)
{
    // zero bits after input exhaustion; the following payload read detects
    // truncation
    unsigned bit = 0;

    if (in->left == 0) {
        if (in->pos == in->len) {
            return 0;
        }
        in->control_at = in->pos;
        in->control = in->src[in->pos++];
        in->left = 8;
    }
    bit = in->control & 1U;
    in->control >>= 1;
    in->left--;
    return bit;
}

// an item as read: a literal (distance 0, its byte in byte), a copy, or the
// end (length 0)
struct item {
    size_t distance;
    size_t length;
    unsigned char byte;
};

static int read_item(struct reader *in, struct item *item)
{
    // the next item's control bits and bytes, or PRSKIT_TRUNCATED where the
    // input ends inside them
    unsigned word = 0;

    if (next_bit(in) == 1) {
        if (in->pos == in->len) {
            return PRSKIT_TRUNCATED;
        }
        item->distance = 0;
        item->length = 1;
        item->byte = in->src[in->pos++];
        return PRSKIT_OK;
    }
    if (next_bit(in) == 0) {
        item->length = next_bit(in) << 1;
        item->length += next_bit(in) + 2;
        if (in->pos == in->len) {
            return PRSKIT_TRUNCATED;
        }
        item->distance = 256 - (size_t)in->src[in->pos++];
        return PRSKIT_OK;
    }
    if (in->len - in->pos < 2) {
        return PRSKIT_TRUNCATED;
    }
    word = in->src[in->pos] | (unsigned)in->src[in->pos + 1] << 8;
    in->pos += 2;
    item->distance = 8192 - (word >> 3);
    item->length = word & 7U;
    if (word == 0) {
        return PRSKIT_OK; // the end
    }
    if (item->length != 0) {
        item->length += 2;
        return PRSKIT_OK;
    }
    if (in->pos == in->len) {
        return PRSKIT_TRUNCATED;
    }
    item->length = in->src[in->pos++] + 1U;
    return PRSKIT_OK;
}

int prskit_decompress(const void *src, size_t len, void *dst, size_t cap,
                      size_t *out, size_t *at)
{
    // payload reads and output writes are bounded by len and cap
    struct reader in = {.src = src, .len = len};
    struct item item = {0, 0, 0};
    unsigned char *to = dst;
    size_t size = 0;
    size_t start = 0;
    int error = PRSKIT_OK;

    if (to == NULL) {
        cap = SIZE_MAX; // measuring, so size_t is the only cap left
    }
    for (;;) {
        start = in.pos;
        error = read_item(&in, &item);
        if (error != PRSKIT_OK || item.length == 0) {
            break;
        }
        if (item.distance > size) {
            error = PRSKIT_BEFORE_START;
            break;
        }
        if (item.length > cap - size) {
            error = PRSKIT_FULL;
            break;
        }
        if (to != NULL && item.distance == 0) {
            to[size] = item.byte;
        } else if (to != NULL) {
            // forward copying allows a back-reference to read bytes
            // produced earlier in the same copy
            for (size_t k = 0; k < item.length; k++) {
                to[size + k] = to[size + k - item.distance];
            }
        }
        size += item.length;
    }
    if (error == PRSKIT_OK) {
        // the end. what's left of its control byte has to be 0, and nothing
        // can come after it
        if (in.control != 0) {
            start = in.control_at;
            error = PRSKIT_PADDING;
        } else {
            start = in.pos;
            error = in.pos == len ? PRSKIT_OK : PRSKIT_TRAILING;
        }
    }
    if (out != NULL) {
        *out = size;
    }
    if (at != NULL) {
        *at = start;
    }
    return error;
}

size_t prskit_bound(size_t len)
{
    // every byte a literal, then the end. the encoder can't do worse: a
    // copy always costs less than the literals it stands for
    size_t control = len / 8; // a control byte for every 8 literals

    // and one for the rest with the end's 2 bits, or two if they spill
    control += len % 8 < 7 ? 1 : 2;
    if (len > SIZE_MAX - control - 2) {
        return 0;
    }
    return len + control + 2;
}

// the encoder's side of the output. a control byte goes in ahead of the
// data its bits describe, and fills in as they arrive
struct writer {
    unsigned char *buf;
    size_t cap;
    size_t size;
    size_t control_at;
    unsigned left;
};

static void put_byte(struct writer *out, unsigned byte)
{
    // past cap it counts the byte and drops it, so a short buffer still
    // learns the size it needed
    if (out->size < out->cap) {
        out->buf[out->size] = (unsigned char)byte;
    }
    out->size++;
}

static inline void put_bit(struct writer *out, unsigned bit)
{
    // meant to stay on the hot path: the encoder calls it for every bit
    if (out->left == 0) {
        out->control_at = out->size;
        put_byte(out, 0);
        out->left = 8;
    }
    if (bit != 0 && out->control_at < out->cap) {
        out->buf[out->control_at] |= (unsigned char)(1U << (8 - out->left));
    }
    out->left--;
}

static void put_literal(struct writer *out, unsigned byte)
{
    put_bit(out, 1);
    put_byte(out, byte);
}

static void put_copy(struct writer *out, size_t distance, size_t length)
{
    unsigned word = (unsigned)(8192 - distance) << 3;

    if (length <= 5 && distance <= 256) {
        put_bit(out, 0);
        put_bit(out, 0);
        put_bit(out, (unsigned)(length - 2) >> 1);
        put_bit(out, (unsigned)(length - 2) & 1U);
        put_byte(out, (unsigned)(256 - distance));
        return;
    }
    put_bit(out, 0);
    put_bit(out, 1);
    if (length <= 9) {
        word |= (unsigned)(length - 2);
    }
    put_byte(out, word & 0xffU);
    put_byte(out, word >> 8);
    if (length > 9) {
        put_byte(out, (unsigned)(length - 1));
    }
}

static void put_end(struct writer *out)
{
    // a long copy with the word 0
    put_bit(out, 0);
    put_bit(out, 1);
    put_byte(out, 0);
    put_byte(out, 0);
}

// match choices inferred from PRS files in SEGA games:
//
// - at each position it takes the longest match, up to 256 bytes, starting
//   at most 8191 back, and the nearest of the longest
// - a two-byte match counts anywhere in the window, but past 256 back no
//   copy can say it, so it goes out as a literal
// - before taking a match it looks one byte ahead. when the match there is
//   at least two bytes longer, it writes a literal and takes that one, and
//   doesn't look ahead again
// - a copy of 2-5 bytes from at most 256 back is short, and the rest are long
//
// the matches come from hash chains over three-byte keys: each position
// links back to the last one whose key hashed the same. a two-byte match,
// wanted only when there's no longer one, is the newest place its pair of
// bytes was seen. head and pair hold positions plus 1, so 0 is none
//
// a run of one byte would make every chain through it as long as the run,
// so a position inside a run links straight back to the run's start, and
// find steps over runs whole
struct encoder {
    const unsigned char *data;
    size_t len;
    size_t inserted;  // positions linked into the chains so far
    size_t run_start; // where the run at inserted began
    size_t head[1 << HASH_BITS];
    size_t pair[1 << 16];
    // how far back each position links, or 0. a window of 8191 behind and
    // the position itself make 8192 slots, used round and round
    unsigned short chain[WINDOW + 1];
};

struct match {
    size_t length;
    size_t distance;
};

static size_t hash(const unsigned char *key)
{
    // knuth's multiplicative hash: three bytes in, HASH_BITS bits out. the
    // math is unsigned long masked to 32 bits, since uint32_t is optional
    unsigned long bytes =
        (unsigned long)key[0] << 16 | (unsigned long)key[1] << 8 | key[2];

    return (size_t)(((bytes * 2654435761UL) & 0xffffffffUL) >>
                    (32 - HASH_BITS));
}

static bool run_of_three(const unsigned char *at)
{
    // three of the same byte: a run, as far as the chains care
    return at[0] == at[1] && at[0] == at[2];
}

static bool inside_run(const unsigned char *data, size_t at, unsigned byte)
{
    // at is inside a run of byte: not its first, and three or more from here
    return at > 0 && data[at - 1] == byte && data[at] == byte &&
           run_of_three(data + at);
}

static void insert_before(struct encoder *enc, size_t end)
{
    // link every position before end into the chains and the pair table.
    // positions arrive in order, so this only ever does new work
    const unsigned char *data = enc->data;

    for (; enc->inserted < end; enc->inserted++) {
        size_t pos = enc->inserted;
        size_t key = 0;
        size_t back = 0;

        if (pos == 0 || data[pos - 1] != data[pos]) {
            enc->run_start = pos;
        }
        if (pos + 2 < enc->len) {
            key = hash(data + pos);
            if (pos > enc->run_start && run_of_three(data + pos)) {
                back = pos - enc->run_start;
            } else if (enc->head[key] != 0) {
                back = pos - (enc->head[key] - 1);
            }
            // a link past the window is no link at all
            enc->chain[pos & WINDOW] =
                (unsigned short)(back <= WINDOW ? back : 0);
            enc->head[key] = pos + 1;
        }
        if (pos + 1 < enc->len) {
            enc->pair[data[pos] << 8 | data[pos + 1]] = pos + 1;
        }
    }
}

static void visit(const unsigned char *data, size_t pos, size_t from,
                  size_t limit, struct match *best)
{
    // keep the match at from if it's longer than best, and three or more:
    // pairs come from the pair table. a longer one agrees on the byte where
    // best runs out, so that byte rules most of them out
    size_t length = 0;

    if (data[from + best->length] != data[pos + best->length]) {
        return;
    }
    while (length < limit && data[from + length] == data[pos + length]) {
        length++;
    }
    if (length > best->length && length >= 3) {
        best->length = length;
        best->distance = pos - from;
    }
}

static struct match find(struct encoder *enc, size_t pos)
{
    // the longest match at pos, and the nearest of those. most of the code
    // is for runs, so a run costs a jump instead of a walk
    const unsigned char *data = enc->data;
    size_t limit = enc->len - pos < MAX_LENGTH ? enc->len - pos : MAX_LENGTH;
    size_t low = pos > WINDOW ? pos - WINDOW : 0;
    size_t run = 0;
    size_t stop = limit;
    size_t from = 0;
    size_t back = 0;
    size_t start = 0;
    size_t end = 0;
    struct match best = {0, 0};

    if (limit < 2) {
        return best;
    }
    insert_before(enc, pos);
    if (limit >= 3 && run_of_three(data + pos)) {
        run = 3;
        while (run < limit && data[pos + run] == data[pos]) {
            run++;
        }
    }

    // pos starts with run copies of a byte. a match longer than that has
    // the same run and then the same byte after it, so it's on the chain of
    // pos+run-2's key, which is short. look there first
    if (run != 0 && run < limit) {
        from = enc->head[hash(data + pos + run - 2)];
    }
    if (from != 0) {
        // from is where the key sits, run-2 bytes into the match
        for (from--; from + 2 >= low + run; from -= back) {
            visit(data, pos, from + 2 - run, limit, &best);
            if (best.length == limit) {
                break;
            }
            back = enc->chain[from & WINDOW];
            if (back == 0) {
                break;
            }
        }
        if (best.length > run) {
            return best;
        }
        best.length = 0;
    }

    // nothing goes past the run, so no match beats run bytes. the search
    // below starts over, finds the nearest that long, and stops there
    if (run != 0) {
        stop = run;
    }

    from = limit >= 3 ? enc->head[hash(data + pos)] : 0;
    if (from != 0) {
        for (from--; from >= low; from -= back) {
            visit(data, pos, from, limit, &best);
            if (best.length >= stop) {
                break;
            }
            back = enc->chain[from & WINDOW];
            if (run != 0 && inside_run(data, from, data[pos])) {
                // from is inside a run of pos's byte, and back leads to its
                // start. a position in that run matches as many bytes as the
                // run has left, but never more than run, so between here and
                // the start only the one with run left can do better. try
                // it, then jump to the start
                start = back != 0 && from - low >= back ? from - back : low;
                end = from + 3;
                while (end - from < run && data[end] == data[pos]) {
                    end++;
                }
                visit(data, pos, end - start >= run ? end - run : start, limit,
                      &best);
                if (best.length >= stop) {
                    break;
                }
                // the run starts outside the window, so that's everything
                if (back == 0 || start != from - back) {
                    break;
                }
                from = start;
                back = enc->chain[from & WINDOW];
            }
            if (back == 0) {
                break;
            }
        }
    }

    // nothing of three or more, so the newest pair, if it's in the window
    if (best.length == 0) {
        from = enc->pair[data[pos] << 8 | data[pos + 1]];
        if (from != 0 && from - 1 >= low) {
            best.length = 2;
            best.distance = pos - (from - 1);
        }
    }
    return best;
}

static void encode(struct encoder *enc, struct writer *out)
{
    // match, look one ahead, then copy or wait. the look-ahead's match is
    // the next position's match, so it's kept, not found twice
    const unsigned char *data = enc->data;
    size_t len = enc->len;
    size_t pos = 0;
    struct match here = {0, 0};
    struct match ahead = {0, 0};
    bool known = false;  // ahead is the match at pos
    bool waited = false; // the last literal waited for ahead

    while (pos < len) {
        here = known ? ahead : find(enc, pos);
        known = false;
        if (here.length != 0 && !waited && pos + 1 < len) {
            ahead = find(enc, pos + 1);
            known = true;
            if (ahead.length >= here.length + 2) {
                put_literal(out, data[pos++]);
                waited = true;
                continue;
            }
        }
        waited = false;
        // a far two-byte match still counts above. it just can't be written
        if (here.length == 0 || (here.length == 2 && here.distance > 256)) {
            put_literal(out, data[pos++]);
            continue;
        }
        put_copy(out, here.distance, here.length);
        pos += here.length;
        known = false; // ahead was inside the copy
    }
}

int prskit_compress(const void *src, size_t len, void *dst, size_t cap,
                    size_t *out)
{
    struct writer stream = {.buf = dst, .cap = cap};
    struct encoder *enc = NULL;

    if (prskit_bound(len) == 0) {
        return PRSKIT_FULL; // compressed-size bound is not representable
    }
    enc = calloc(1, sizeof *enc);
    if (enc == NULL) {
        return PRSKIT_NOMEM;
    }
    enc->data = src;
    enc->len = len;
    encode(enc, &stream);
    put_end(&stream);
    free(enc);
    if (out != NULL) {
        *out = stream.size;
    }
    return stream.size <= cap ? PRSKIT_OK : PRSKIT_FULL;
}

const char *prskit_strerror(int error)
{
    switch (error) {
    case PRSKIT_OK:
        return "ok";
    case PRSKIT_TRUNCATED:
        return "the input ends before the end marker";
    case PRSKIT_BEFORE_START:
        return "a copy reaches back past the start of the output";
    case PRSKIT_PADDING:
        return "the last control byte has bits set after the end";
    case PRSKIT_TRAILING:
        return "bytes follow the end marker";
    case PRSKIT_FULL:
        return "the output doesn't fit";
    case PRSKIT_NOMEM:
        return "out of memory";
    default:
        return "unknown error";
    }
}
