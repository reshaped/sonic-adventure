/* GD-ROM's file system, over the disc's root directory. a read is done when
 * its call returns. so a request's state is complete or failed at once, and
 * the drive is never open or busy. names match without case, as ISO 9660's
 * upper case and the game's lower case differ */
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#ifdef _WIN32
#include <windows.h>
/* minwindef.h's empty near and far would erase sa_engine.h's fields */
#undef near
#undef far
#endif

#include "native.h"
#include "sa_engine.h"

#define GDD_ERR_OK 0
#define GDD_ERR_NOTFOUND (-8)
#define GDD_ERR_ILLHNDL (-10)
#define GDD_ERR_NOTDIR (-11)
#define GDD_ERR_MEDIA (-30)
#define GDD_ERR_ILLREQ (-32)
#define GDD_STAT_IDLE 0
#define GDD_STAT_COMPLETE 1
#define GDD_STAT_ERR 5
#define GDD_DRVSTAT_PAUSE 1
#define SECTOR 2048
/* the sectors of the largest file a handle counts */
#define MOST_SECTORS (INT32_MAX / SECTOR + 1)
#define FILES 16

/* a file handle, closed while file is null */
struct gdfs {
    FILE *file;
    /* the file's bytes as it opened */
    int32_t size;
    /* the sector the next read starts at */
    int32_t next;
    /* how the last read ended, a GDD_STAT_ value, idle before the first */
    int32_t stat;
    /* the name the game opened it by, for a failed read's report */
    char name[64];
};
static struct gdfs files[FILES];
/* the current directory, below the root, as the disc names it; empty at
 * the root */
static char directory[256];
char native_last_opened[64];
/* the file whose failed read stands reported. the game's queue asks again for
 * a failed read. so a file's failure prints one line until a read of it
 * succeeds */
static char failing[64];

/* name's entry in the host's directory folder, as the disc spells it, in
 * out */
static int entry_nocase(const char *folder, const char *name, char *out,
                        size_t size)
{
    DIR *dir;
    struct dirent *entry;
    int found = 0;

    if (!(dir = opendir(folder))) {
        return 0;
    }
    while ((entry = readdir(dir))) {
        if (!strcasecmp(entry->d_name, name)) {
            snprintf(out, size, "%s/%s", folder, entry->d_name);
            found = 1;
            break;
        }
    }
    closedir(dir);
    return found;
}

/* name's entry in the current directory, as the disc spells it, in out */
static int find_nocase(const char *name, char *out, size_t size)
{
    char path[512];

    snprintf(path, sizeof path, "%s/%s", native_disc, directory);
    return entry_nocase(path, name, out, size);
}

FILE *native_disc_file(const char *name)
{
    char path[1024];

    return find_nocase(name, path, sizeof path) ? fopen(path, "rb") : NULL;
}

uint32_t native_disc_file_id(const char *name)
{
    char path[512];

    snprintf(path, sizeof path, "%s%s%s", directory, directory[0] ? "/" : "",
             name);
    for (char *at = path; *at; at++) {
        *at = (char)toupper((unsigned char)*at);
    }
    return sa_name_id(path);
}

/* x rotated right */
static uint32_t turned(uint32_t x, int by)
{
    return x >> by | x << (32 - by);
}

/* SHA-256 (FIPS 180-4), as SHA256SUMS sums the build's disc. this folds a
 * block's 64 bytes into the state's eight words */
static void sum_block(uint32_t state[8], const uint8_t *block)
{
    static const uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
        0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
        0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
        0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
        0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
        0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
    uint32_t w[64], first, second;
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

    for (int i = 0; i < 16; i++) {
        w[i] = (uint32_t)block[4 * i] << 24 | (uint32_t)block[4 * i + 1] << 16 |
               (uint32_t)block[4 * i + 2] << 8 | block[4 * i + 3];
    }
    for (int i = 16; i < 64; i++) {
        first = w[i - 15];
        second = w[i - 2];
        w[i] = w[i - 16] + (turned(first, 7) ^ turned(first, 18) ^ first >> 3) +
               w[i - 7] +
               (turned(second, 17) ^ turned(second, 19) ^ second >> 10);
    }
    for (int i = 0; i < 64; i++) {
        first = h + (turned(e, 6) ^ turned(e, 11) ^ turned(e, 25)) +
                ((e & f) ^ (~e & g)) + k[i] + w[i];
        second = (turned(a, 2) ^ turned(a, 13) ^ turned(a, 22)) +
                 ((a & b) ^ (a & c) ^ (b & c));
        h = g;
        g = f;
        f = e;
        e = d + first;
        d = c;
        c = b;
        b = a;
        a = first + second;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

/* file's SHA-256 from its position to its end, in sum as SHA256SUMS writes
 * one. it gives 0 where the host fails a read */
static int sum_file(FILE *file, char sum[65])
{
    static uint8_t bytes[1 << 16];
    uint32_t state[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                         0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    uint8_t last[128] = {0};
    uint64_t bits = 0;
    size_t held, at, left, blocks;

    do {
        held = fread(bytes, 1, sizeof bytes, file);
        bits += (uint64_t)held * 8;
        for (at = 0; at + 64 <= held; at += 64) {
            sum_block(state, bytes + at);
        }
    } while (held == sizeof bytes);
    if (ferror(file)) {
        return 0;
    }
    /* the bytes left and a 1 bit, then zeros up to the count of bits. the
     * count ends a block, high byte first */
    left = held - at;
    memcpy(last, bytes + at, left);
    last[left] = 0x80;
    blocks = left < 56 ? 1 : 2;
    for (int i = 0; i < 8; i++) {
        last[blocks * 64 - 1 - (size_t)i] = (uint8_t)(bits >> (8 * i));
    }
    for (at = 0; at < blocks; at++) {
        sum_block(state, last + 64 * at);
    }
    for (int i = 0; i < 8; i++) {
        snprintf(sum + 8 * i, 9, "%08x", (unsigned)state[i]);
    }
    return 1;
}

/* in out, the host's folder for the directory in path's first held bytes, as
 * the disc spells it */
static int folder_nocase(const char *path, size_t held, char *out, size_t size)
{
    char name[256], next[1024];
    size_t end;

    snprintf(out, size, "%s", native_disc);
    for (size_t at = 0; at < held; at = end + 1) {
        for (end = at; end < held && path[end] != '/'; end++) {
        }
        snprintf(name, sizeof name, "%.*s", (int)(end - at), path + at);
        if (!entry_nocase(out, name, next, sizeof next)) {
            return 0;
        }
        snprintf(out, size, "%s", next);
    }
    return 1;
}

/* path's name, where path lies in the directory that is row's first held
 * bytes, and else null */
static const char *name_beside(const char *path, const char *row, size_t held)
{
    const char *name;

    if (strncmp(path, row, held) != 0 || (held && path[held] != '/')) {
        return NULL;
    }
    name = path + held + (held != 0);
    return strchr(name, '/') ? NULL : name;
}

/* what the disc holds of a file of the build's disc. a positive value is the
 * errno of its failed read */
enum { DISC_UNREAD = 0, DISC_LACKING = -1, DISC_OTHER = -2, DISC_SAME = -3 };

/* the build compiled its disc's files into this program. on a disc that holds
 * others, the game's code is one disc's and its files another's. each
 * directory is read once. where two entries spell one name in two cases, the
 * first is the file, as in entry_nocase */
int native_disc_differs(char *words, size_t size)
{
    int count = native_disc_sum_count, first;
    int *found = calloc((size_t)count, sizeof *found);
    char folder[1024], path[1280], sum[65];

    if (count && !found) {
        snprintf(words, size, "no memory to read the disc at %s", native_disc);
        return 1;
    }
    for (int i = 0; i < count; i++) {
        const char *row = native_disc_sums[i].path, *slash = strrchr(row, '/');
        size_t held = slash ? (size_t)(slash - row) : 0;
        const char *name;
        struct dirent *entry;
        DIR *dir;
        FILE *file;

        if (found[i] != DISC_UNREAD) {
            continue;
        }
        dir = folder_nocase(row, held, folder, sizeof folder) ? opendir(folder)
                                                              : NULL;
        while (dir && (entry = readdir(dir))) {
            for (int j = i; j < count; j++) {
                if (found[j] != DISC_UNREAD ||
                    !(name =
                          name_beside(native_disc_sums[j].path, row, held)) ||
                    strcasecmp(entry->d_name, name) != 0) {
                    continue;
                }
                snprintf(path, sizeof path, "%s/%s", folder, entry->d_name);
                errno = 0;
                if (!(file = fopen(path, "rb")) || !sum_file(file, sum)) {
                    found[j] = errno > 0 ? errno : EIO;
                } else {
                    found[j] = strcmp(sum, native_disc_sums[j].sum) ? DISC_OTHER
                                                                    : DISC_SAME;
                }
                if (file) {
                    fclose(file);
                }
            }
        }
        if (dir) {
            closedir(dir);
        }
        for (int j = i; j < count; j++) {
            if (found[j] == DISC_UNREAD &&
                name_beside(native_disc_sums[j].path, row, held)) {
                found[j] = DISC_LACKING;
            }
        }
    }
    for (first = 0; first < count && found[first] == DISC_SAME; first++) {
    }
    if (first < count && found[first] == DISC_LACKING) {
        snprintf(words, size, "the disc at %s lacks %s", native_disc,
                 native_disc_sums[first].path);
    } else if (first < count && found[first] == DISC_OTHER) {
        snprintf(words, size,
                 "the disc at %s holds another %s than the disc this program "
                 "was built from",
                 native_disc, native_disc_sums[first].path);
    } else if (first < count) {
        snprintf(words, size, "can't read %s on the disc at %s: %s",
                 native_disc_sums[first].path, native_disc,
                 strerror(found[first]));
    }
    free(found);
    return first < count;
}

int32_t gdFsInit(int32_t max_open, void *work, int32_t max_dirent, void *dirbuf)
{
    (void)max_open, (void)work, (void)max_dirent, (void)dirbuf;
    return GDD_ERR_OK;
}

/* the root, the directory above, or name's in the current directory. a missing
 * name gives GDD_ERR_NOTFOUND and a file's gives GDD_ERR_NOTDIR, as the SDK's
 * gdFsLoadDir does. either leaves the current directory as it was */
int32_t gdFsChangeDir(const char *name)
{
    char path[1024];
    char *last;
    DIR *dir;

    if (!strcmp(name, "\\")) {
        directory[0] = 0;
    } else if (!strcmp(name, "..")) {
        last = strrchr(directory, '/');
        *(last ? last : directory) = 0;
    } else {
        size_t used = strlen(directory);

        if (!find_nocase(name, path, sizeof path)) {
            return GDD_ERR_NOTFOUND;
        }
        if (!(dir = opendir(path))) {
            return GDD_ERR_NOTDIR;
        }
        closedir(dir);
        snprintf(directory + used, sizeof directory - used, "%s%s",
                 used ? "/" : "", strrchr(path, '/') + 1);
    }
    return GDD_ERR_OK;
}

/* report a read of name's that the host failed, and why */
static void cant_read(const char *name, const char *why)
{
    fprintf(stderr, "sa1: can't read %s on the disc: %s\n", name, why);
}

/* a handle holds the file's size in the SDK's 32 bits. a file the host won't
 * size, or sizes past them, doesn't open */
struct gdfs *gdFsOpen(const char *name, void *dirrec)
{
    char path[1024];
    FILE *file;
    long size = 0;
    const char *why = NULL;

    (void)dirrec;
    snprintf(native_last_opened, sizeof native_last_opened, "%s", name);
    if (!find_nocase(name, path, sizeof path) || !(file = fopen(path, "rb"))) {
        fprintf(stderr, "sa1: no file %s on the disc\n", name);
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0) {
        why = strerror(errno);
    } else if (size > INT32_MAX) {
        why = "it is larger than the 2 GB a handle counts";
    }
    if (why) {
        cant_read(name, why);
        fclose(file);
        return NULL;
    }
    for (int i = 0; i < FILES; i++) {
        if (!files[i].file) {
            files[i].file = file;
            files[i].size = (int32_t)size;
            files[i].next = 0;
            files[i].stat = GDD_STAT_IDLE;
            snprintf(files[i].name, sizeof files[i].name, "%s", name);
            return &files[i];
        }
    }
    fclose(file);
    return NULL;
}

int gdFsGetFileSize(struct gdfs *gdfs, int32_t *size)
{
    if (!gdfs || !gdfs->file) {
        return 0;
    }
    *size = gdfs->size;
    return 1;
}

/* whether at lies in the executable's code. the game reads MOVIE.BIN over
 * movie_entry, which the slice links from engine/sa1/movie.c. a read into code
 * writes nothing. Windows gives the image's code pages, where Linux's link
 * names the code's bounds */
#ifdef _WIN32
static int in_code(const void *at)
{
    MEMORY_BASIC_INFORMATION pages;

    return VirtualQuery(at, &pages, sizeof pages) && pages.Type == MEM_IMAGE &&
           (pages.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ |
                             PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY));
}
#else
extern char __executable_start[], etext[];

static int in_code(const void *at)
{
    return (const char *)at >= __executable_start && (const char *)at < etext;
}
#endif

/* read sectors from the handle's next sector into buffer, up to the file's end
 * as the SDK's does. so no count writes past the file's last sector. a count
 * below zero or above MOST_SECTORS is GDD_ERR_ILLREQ, the SDK's name for the
 * drive's illegal request. a failed read or seek, or a file shorter than its
 * size at open, is GDD_ERR_MEDIA. the buffer keeps what arrived. after either
 * error the handle's state is GDD_STAT_ERR, which the game's queue asks again
 * on. its next sector stays, so the next read takes the same sectors */
int32_t gdFsRead(struct gdfs *gdfs, int32_t sectors, void *buffer)
{
    int32_t left;
    size_t from, bytes, held;
    const char *why = NULL;

    if (!gdfs || !gdfs->file) {
        return GDD_ERR_ILLHNDL;
    }
    if (sectors < 0 || sectors > MOST_SECTORS) {
        gdfs->stat = GDD_STAT_ERR;
        return GDD_ERR_ILLREQ;
    }
    if (in_code(buffer)) {
        gdfs->stat = GDD_STAT_COMPLETE;
        return GDD_ERR_OK;
    }
    left = gdfs->size / SECTOR + (gdfs->size % SECTOR != 0) - gdfs->next;
    if (sectors > left) {
        sectors = left;
    }
    if (sectors == 0) {
        gdfs->stat = GDD_STAT_COMPLETE;
        return GDD_ERR_OK;
    }
    /* at least one sector is left, so from lies inside the file */
    from = (size_t)gdfs->next * SECTOR;
    bytes = (size_t)sectors * SECTOR;
    held = (size_t)gdfs->size - from;
    if (held > bytes) {
        held = bytes;
    }
    if (fseek(gdfs->file, (long)from, SEEK_SET) != 0) {
        why = strerror(errno);
    } else if (fread(buffer, 1, held, gdfs->file) != held) {
        why = ferror(gdfs->file) ? strerror(errno)
                                 : "it ends before the size it opened with";
    }
    if (why) {
        if (strcasecmp(failing, gdfs->name) != 0) {
            cant_read(gdfs->name, why);
            snprintf(failing, sizeof failing, "%s", gdfs->name);
        }
        clearerr(gdfs->file);
        gdfs->stat = GDD_STAT_ERR;
        return GDD_ERR_MEDIA;
    }
    if (!strcasecmp(failing, gdfs->name)) {
        failing[0] = 0;
    }
    /* the drive writes whole sectors: a short last one ends in zeros here */
    memset((char *)buffer + held, 0, bytes - held);
    gdfs->next += sectors;
    gdfs->stat = GDD_STAT_COMPLETE;
    return GDD_ERR_OK;
}

/* the SDK's request returns the sectors it takes, 0 at the file's end. here
 * the read is over when it returns, so a failed one returns its error */
int32_t gdFsReqRd32(struct gdfs *gdfs, int32_t sectors, void *buffer)
{
    int32_t from = gdfs ? gdfs->next : 0;
    int32_t result = gdFsRead(gdfs, sectors, buffer);

    return result == GDD_ERR_OK ? gdfs->next - from : result;
}

int32_t gdFsGetStat(struct gdfs *gdfs)
{
    return gdfs && gdfs->file ? gdfs->stat : GDD_ERR_ILLHNDL;
}

void gdFsClose(struct gdfs *gdfs)
{
    if (gdfs && gdfs->file) {
        fclose(gdfs->file);
        gdfs->file = NULL;
    }
}

int32_t gdFsGetDrvStat(void)
{
    return GDD_DRVSTAT_PAUSE;
}
