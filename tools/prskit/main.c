// prskit command-line interface
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prskit.h"

static const char usage[] = "usage: prskit compress IN OUT\n"
                            "       prskit decompress IN OUT\n"
                            "       prskit check FILE...\n";

// BAD is invalid data or a mismatch; FAILED is an operational error
enum { OK, BAD, FAILED };

struct tally {
    size_t exact;
    size_t differ;
    size_t broken;
};

static void complain(const char *path)
{
    fprintf(stderr, "prskit: %s: %s\n", path,
            errno != 0 ? strerror(errno) : "input/output error");
}

static int read_file(const char *path, unsigned char **data, size_t *len)
{
    // the whole file, into a buffer that doubles until it's big enough
    FILE *file = NULL;
    unsigned char *buf = NULL;
    unsigned char *grown = NULL;
    size_t size = 0;
    size_t cap = 0;
    bool ok = true;

    errno = 0;
    file = fopen(path, "rb");
    if (file == NULL) {
        complain(path);
        return -1;
    }
    // a full buffer means the file may go on
    while (ok && size == cap) {
        cap = cap != 0 ? cap * 2 : (size_t)1 << 16;
        grown = cap > size ? realloc(buf, cap) : NULL; // unless cap wrapped
        if (grown == NULL) {
            // C99 does not require realloc to set errno
            fprintf(stderr, "prskit: %s: out of memory\n", path);
            ok = false;
        } else {
            buf = grown;
            size += fread(buf + size, 1, cap - size, file);
        }
    }
    if (ok && ferror(file) != 0) {
        complain(path);
        ok = false;
    }
    fclose(file);
    if (!ok) {
        free(buf);
        return -1;
    }
    *data = buf;
    *len = size;
    return 0;
}

static int write_file(const char *path, const unsigned char *data, size_t len)
{
    // fclose can report buffered write errors. leave partial files in place:
    // the destination might be a device or symlink
    FILE *file = NULL;
    bool ok = false;

    errno = 0;
    file = fopen(path, "wb");
    if (file == NULL) {
        complain(path);
        return -1;
    }
    ok = fwrite(data, 1, len, file) == len;
    ok = fclose(file) == 0 && ok;
    if (!ok) {
        complain(path);
    }
    return ok ? 0 : -1;
}

static int decode(const unsigned char *in, size_t len, unsigned char **out,
                  size_t *size, size_t *at)
{
    // measure before allocating the decoded buffer. PRSKIT_FULL here means the
    // output is larger than SIZE_MAX, which no allocation can hold
    int error = prskit_decompress(in, len, NULL, 0, size, at);

    if (error == PRSKIT_FULL) {
        return PRSKIT_NOMEM;
    }
    if (error != PRSKIT_OK) {
        return error;
    }
    *out = malloc(*size != 0 ? *size : 1);
    if (*out == NULL) {
        return PRSKIT_NOMEM;
    }
    return prskit_decompress(in, len, *out, *size, size, at);
}

static int encode(const unsigned char *in, size_t len, unsigned char **out,
                  size_t *size)
{
    // prskit_bound bytes always hold the stream: no PRSKIT_FULL here
    size_t bound = prskit_bound(len);

    if (bound == 0) {
        return PRSKIT_NOMEM; // no representable output bound
    }
    *out = malloc(bound);
    if (*out == NULL) {
        return PRSKIT_NOMEM;
    }
    return prskit_compress(in, len, *out, bound, size);
}

static int compress_file(const char *from, const char *to)
{
    unsigned char *in = NULL;
    unsigned char *out = NULL;
    size_t len = 0;
    size_t size = 0;
    int error = PRSKIT_OK;
    int status = FAILED;

    if (read_file(from, &in, &len) != 0) {
        return FAILED;
    }
    error = encode(in, len, &out, &size);
    if (error != PRSKIT_OK) {
        fprintf(stderr, "prskit: %s: %s\n", from, prskit_strerror(error));
    } else if (write_file(to, out, size) == 0) {
        status = OK;
    }
    free(in);
    free(out);
    return status;
}

static int decompress_file(const char *from, const char *to)
{
    unsigned char *in = NULL;
    unsigned char *out = NULL;
    size_t len = 0;
    size_t size = 0;
    size_t at = 0;
    int error = PRSKIT_OK;
    int status = FAILED;

    if (read_file(from, &in, &len) != 0) {
        return FAILED;
    }
    error = decode(in, len, &out, &size, &at);
    if (error == PRSKIT_NOMEM) {
        fprintf(stderr, "prskit: %s: %s\n", from, prskit_strerror(error));
    } else if (error != PRSKIT_OK) {
        fprintf(stderr, "prskit: %s: %s at 0x%zx\n", from,
                prskit_strerror(error), at);
        status = BAD;
    } else if (write_file(to, out, size) == 0) {
        status = OK;
    }
    free(in);
    free(out);
    return status;
}

static int check_file(const char *path, struct tally *seen)
{
    unsigned char *in = NULL;
    unsigned char *image = NULL;
    unsigned char *again = NULL;
    size_t len = 0;
    size_t size = 0;
    size_t encoded = 0;
    size_t at = 0;
    size_t same = 0;
    int error = PRSKIT_OK;
    int status = OK;

    if (read_file(path, &in, &len) != 0) {
        return FAILED;
    }
    error = decode(in, len, &image, &size, &at);
    if (error == PRSKIT_OK) {
        error = encode(image, size, &again, &encoded);
    }
    if (error == PRSKIT_NOMEM) {
        fprintf(stderr, "prskit: %s: %s\n", path, prskit_strerror(error));
        status = FAILED;
    } else if (error != PRSKIT_OK) {
        printf("error    %s: %s at 0x%zx\n", path, prskit_strerror(error), at);
        seen->broken++;
    } else if (encoded == len && memcmp(again, in, len) == 0) {
        printf("exact    %s\n", path);
        seen->exact++;
    } else {
        while (same < len && same < encoded && again[same] == in[same]) {
            same++;
        }
        printf("differs  %s: at 0x%zx of 0x%zx (encoded 0x%zx)\n", path, same,
               len, encoded);
        seen->differ++;
    }
    free(in);
    free(image);
    free(again);
    return status;
}

int main(int argc, char **argv)
{
    const char *command = argc > 1 ? argv[1] : "";
    int files = argc - 2;
    int status = OK;
    struct tally seen = {0};

    if (files == 2 && strcmp(command, "compress") == 0) {
        return compress_file(argv[2], argv[3]);
    }
    if (files == 2 && strcmp(command, "decompress") == 0) {
        return decompress_file(argv[2], argv[3]);
    }
    if (files < 1 || strcmp(command, "check") != 0) {
        fputs(usage, stderr);
        return FAILED;
    }
    for (int i = 2; i < argc; i++) {
        if (check_file(argv[i], &seen) != OK) {
            status = FAILED; // one failed file shouldn't hide the rest
        }
    }
    printf("%d file%s: %zu exact, %zu differ, %zu don't decode\n", files,
           files == 1 ? "" : "s", seen.exact, seen.differ, seen.broken);

    // report buffered stdout errors
    errno = 0;
    if (fflush(stdout) != 0 || ferror(stdout) != 0) {
        complain("stdout");
        return FAILED;
    }
    if (status != OK) {
        return status;
    }
    return seen.exact == (size_t)files ? OK : BAD;
}
