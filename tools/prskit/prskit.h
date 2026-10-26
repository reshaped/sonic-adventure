// PRS compression and decompression
#ifndef PRSKIT_H
#define PRSKIT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    PRSKIT_OK,
    PRSKIT_TRUNCATED,    // the input ends before the end marker
    PRSKIT_BEFORE_START, // a copy reaches back past the start of the output
    PRSKIT_PADDING,      // the last control byte has bits set after the end
    PRSKIT_TRAILING,     // bytes follow the end marker
    PRSKIT_FULL,         // the output needs more than cap bytes
    PRSKIT_NOMEM,        // the encoder can't allocate its tables
};

// requires 8-bit bytes and 32-bit or wider int and size_t. src must address
// len readable bytes (or may be NULL when len is zero). non-NULL dst must
// address cap writable bytes, and src and dst must not overlap. out and at,
// when supplied, must point to writable size_t objects.

// decode the stream in src[0..len) into dst[0..cap). *out gets the bytes
// produced before the end or the item that fails, and *at where the stream
// ends or the input offset of the item that fails: for PRSKIT_PADDING, its
// control byte. either can be null
//
// with dst null it measures without writing, and cap is ignored. if the
// decoded size would pass SIZE_MAX it returns PRSKIT_FULL, and *out is the
// size measured before the item that would pass it. callers should impose
// a size limit before allocating for untrusted input. PRSKIT_TRAILING leaves
// the output complete and *at at the stream end
int prskit_decompress(const void *src, size_t len, void *dst, size_t cap,
                      size_t *out, size_t *at);

// the most the encoder writes for len bytes, which is all of them as
// literals. 0 if that doesn't fit in a size_t
size_t prskit_bound(size_t len);

// encode src[0..len) into dst[0..cap), using match choices checked against
// SEGA game PRS files. *out, if not null, gets the stream's size, even on
// PRSKIT_FULL, but not on PRSKIT_NOMEM or when prskit_bound(len) is 0. dst can
// be null when cap is 0. each call allocates and frees about 800 KB of tables,
// half that with a 32-bit size_t
int prskit_compress(const void *src, size_t len, void *dst, size_t cap,
                    size_t *out);

// what an error code means, in a few words
const char *prskit_strerror(int error);

#ifdef __cplusplus
}
#endif

#endif
