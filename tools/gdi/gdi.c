// Read and rebuild Dreamcast GDI images. The .gdi locates the tracks;
// ISO9660 begins in the high-density area at LBA 45000.

// POSIX/XSI file APIs and 64-bit offsets are required for large tracks.
// NOLINTBEGIN(bugprone-reserved-identifier,readability-identifier-naming)
#define _XOPEN_SOURCE 700
#define _FILE_OFFSET_BITS 64
// NOLINTEND(bugprone-reserved-identifier,readability-identifier-naming)

#include "gdi.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

enum {
    BLOCK = 2048,      // ISO9660 data bytes per sector
    RAW = 2352,        // raw sector including header and error codes
    HIGH = 45000,      // high-density area begins here
    SECTORS = 1 << 20, // upper bound, beyond any GD-ROM
    TRACKS = 99,
    DEPTH = 64, // maximum directory nesting
    PATH = 4096,
    LINE = PATH + 64,
    LATE = 450000,   // minimum start of a relocated boot file
    CHUNK = 1 << 16, // rebuild copy buffer
};

// Track extents begin at index 1, excluding pregaps.
struct track {
    uint32_t start;  // first absolute sector (LBA)
    uint32_t count;  // sectors; 0 for audio, which only rebuild counts
    uint64_t offset; // bytes preceding the track's first sector
    unsigned size;   // stored sector size
    bool data;       // false for audio
    bool raw;        // 2352-byte sectors instead of 2048-byte payloads
    FILE *file;      // data track opened during gdi_open
    char *path;      // resolved alongside the .gdi
};

// ISO9660 entry without its version suffix
struct entry {
    uint32_t sector;
    uint32_t size;
    bool directory;
    char name[256];
};

struct gdi {
    struct track tracks[TRACKS];
    size_t count;
    struct entry root;
    FILE *file;                      // the original .gdi, kept open
    unsigned char seen[SECTORS / 8]; // directory-cycle detection
    char image[PATH];                // the .gdi's path as supplied
    char failed[PATH];               // path associated with the last error
};

struct cursor {
    uint32_t sector; // absolute LBA of block
    uint32_t left;   // directory bytes from this block on
    size_t at;       // next record offset within block
    unsigned char block[BLOCK];
};

struct walk {
    char path[PATH];
    bool extract; // listing with a null callback still never writes
    void (*each)(void *context, const char *path, uint32_t size);
    void *context;
};

static int fail(struct gdi *disc, int error, const char *path)
{
    // retain the failing path without disturbing the caller's errno
    int saved = errno;
    size_t length = strlen(path);
    if (length >= PATH) {
        length = PATH - 1;
    }
    memcpy(disc->failed, path, length);
    disc->failed[length] = '\0';
    errno = saved;
    return error;
}

static uint32_t le32(const unsigned char *at)
{
    // ISO9660 stores both byte orders; read its little-endian half
    return (uint32_t)at[0] | (uint32_t)at[1] << 8 | (uint32_t)at[2] << 16 |
           (uint32_t)at[3] << 24;
}

static int number(const char **text, uint64_t *value)
{
    // reject signs and trailing junk that strtoull otherwise accepts
    const char *at = *text + strspn(*text, " \t");
    char *end = NULL;
    if (*at < '0' || *at > '9') {
        return GDI_INVALID;
    }
    errno = 0;
    unsigned long long parsed = strtoull(at, &end, 10);
    if (errno != 0 || (*end != '\0' && *end != ' ' && *end != '\t')) {
        return GDI_INVALID;
    }
    *value = parsed;
    *text = end;
    return GDI_OK;
}

static int fetch(const struct track *track, uint32_t at, unsigned char out[RAW])
{
    // fseeko clears EOF, but not an earlier read's error indicator
    clearerr(track->file);
    if (fseeko(track->file,
               (off_t)(track->offset + ((uint64_t)at * track->size)),
               SEEK_SET) != 0) {
        return GDI_READ;
    }
    if (fread(out, track->size, 1, track->file) != 1) {
        return ferror(track->file) != 0 ? GDI_READ : GDI_INVALID;
    }
    return GDI_OK;
}

static unsigned bcd(uint32_t value)
{
    return ((value / 10 % 10) << 4) | (value % 10);
}

static const struct track *locate(const struct gdi *disc, uint32_t number)
{
    for (size_t i = 0; i < disc->count; i++) {
        const struct track *track = &disc->tracks[i];
        if (track->data && number >= track->start &&
            number - track->start < track->count) {
            return track;
        }
    }
    return NULL;
}

static bool sector_header_matches(const unsigned char in[RAW], uint32_t number)
{
    // addresses run 150 frames ahead of LBAs. Ignore the minute: dumps
    // encode minutes beyond 99 inconsistently.
    static const unsigned char sync[12] = {0,   255, 255, 255, 255, 255,
                                           255, 255, 255, 255, 255, 0};
    uint32_t frames = number + 150;
    return memcmp(in, sync, sizeof sync) == 0 &&
           in[13] == bcd(frames / 75 % 60) && in[14] == bcd(frames % 75);
}

static int sector(struct gdi *disc, uint32_t number, unsigned char out[BLOCK])
{
    // extract 2048 data bytes. Raw mode 1 uses offset 16; mode 2 form 1
    // uses offset 24. Both require the expected sync and address.
    const struct track *track = locate(disc, number);
    if (track == NULL) {
        return GDI_INVALID;
    }
    unsigned char in[RAW] = {0};
    int error = fetch(track, number - track->start, in);
    if (error != GDI_OK) {
        (void)fail(disc, error, track->path);
        return error;
    }
    bool here = sector_header_matches(in, number);
    if (!track->raw) {
        memcpy(out, in, BLOCK);
    } else if (here && in[15] == 1) {
        memcpy(out, in + 16, BLOCK);
    } else if (here && in[15] == 2 && (in[18] & 0x20U) == 0) {
        memcpy(out, in + 24, BLOCK);
    } else {
        // form 2 has 2324 data bytes, not an ISO9660 block
        (void)fail(disc, GDI_INVALID, track->path);
        return GDI_INVALID;
    }
    return GDI_OK;
}

static int open_file(const char *path, FILE **file, struct stat *st)
{
    // reject nonregular inputs. O_NONBLOCK prevents FIFO hangs;
    // O_CLOEXEC prevents descriptor inheritance.
    int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        return GDI_READ;
    }
    int error = GDI_OK;
    if (fstat(fd, st) != 0) {
        error = GDI_READ;
    } else if (!S_ISREG(st->st_mode)) {
        error = GDI_INVALID;
    } else {
        *file = fdopen(fd, "rb");
        error = *file != NULL ? GDI_OK : GDI_READ;
    }
    if (error != GDI_OK) {
        int saved = errno;
        (void)close(fd);
        errno = saved;
    }
    return error;
}

static int open_track(const struct track *track, FILE **file, uint32_t *count)
{
    // bytes after the offset must be complete sectors within LBA bounds
    struct stat st;
    int error = open_file(track->path, file, &st);
    if (error != GDI_OK) {
        return error;
    }
    uint64_t bytes = (uint64_t)st.st_size;
    if (bytes < track->offset || (bytes - track->offset) % track->size != 0 ||
        (bytes - track->offset) / track->size > SECTORS - track->start) {
        (void)fclose(*file);
        *file = NULL;
        return GDI_INVALID;
    }
    *count = (uint32_t)((bytes - track->offset) / track->size);
    return GDI_OK;
}

static int create(const char *path, FILE **file)
{
    // O_EXCL never overwrites; unlink a new file if fdopen fails
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0666);
    *file = fd >= 0 ? fdopen(fd, "wb") : NULL;
    if (*file == NULL) {
        int saved = errno;
        if (fd >= 0) {
            (void)close(fd);
            (void)unlink(path);
        }
        errno = saved;
        return GDI_WRITE;
    }
    return GDI_OK;
}

static int gdi_track(struct gdi *disc, const char *line, const char *image,
                     size_t folder)
{
    // number, first LBA, type (4 data / 0 audio), stored sector size,
    // quoted or bare filename, and optional byte offset:
    //   3 45000 4 2352 "Disc (Track 3).bin" 0
    uint64_t index;
    uint64_t start;
    uint64_t type;
    uint64_t size;
    uint64_t offset = 0;
    const char *at = line;
    if (number(&at, &index) != GDI_OK || number(&at, &start) != GDI_OK ||
        number(&at, &type) != GDI_OK || number(&at, &size) != GDI_OK) {
        return GDI_INVALID;
    }
    at += strspn(at, " \t");
    bool quoted = *at == '"';
    const char *name = quoted ? ++at : at;
    at += strcspn(at, quoted ? "\"" : " \t");
    size_t length = (size_t)(at - name);
    if (quoted) {
        if (*at != '"') {
            return GDI_INVALID;
        }
        at++;
    }
    at += strspn(at, " \t");
    if (*at != '\0' && number(&at, &offset) != GDI_OK) {
        return GDI_INVALID;
    }
    at += strspn(at, " \t");
    if (*at != '\0') {
        return GDI_INVALID;
    }
    if (index != disc->count + 1 || start >= SECTORS || length == 0 ||
        memchr(name, '/', length) != NULL || (type != 0 && type != 4) ||
        (size != BLOCK && size != RAW) || folder + length >= PATH) {
        return GDI_INVALID;
    }
    struct track *track = &disc->tracks[disc->count++];
    track->start = (uint32_t)start;
    track->offset = offset;
    track->size = (unsigned)size;
    track->data = type == 4;
    track->raw = size == RAW;
    track->path = malloc(folder + length + 1);
    if (track->path == NULL) {
        return GDI_NOMEM;
    }
    char *path = track->path;
    memcpy(path, image, folder);
    memcpy(path + folder, name, length);
    path[folder + length] = '\0';
    if (!track->data) {
        return GDI_OK; // audio is opened only for rebuild
    }
    int error = open_track(track, &track->file, &track->count);
    return error != GDI_OK ? fail(disc, error, path) : GDI_OK;
}

static int gdi_line(FILE *gdi, char line[LINE])
{
    // reject overlong lines rather than parsing a prefix
    if (fgets(line, LINE, gdi) == NULL) {
        return ferror(gdi) != 0 ? GDI_READ : GDI_INVALID;
    }
    char *end = strpbrk(line, "\r\n");
    if (end == NULL && feof(gdi) == 0) {
        return GDI_INVALID;
    }
    if (end != NULL) {
        *end = '\0';
    }
    return GDI_OK;
}

static int gdi_lines(struct gdi *disc, FILE *gdi, const char *image)
{
    // read the declared number of track records, skipping blank lines
    char line[LINE];
    const char *at = line;
    uint64_t count;
    int error = gdi_line(gdi, line);
    if (error != GDI_OK) {
        return error;
    }
    if (number(&at, &count) != GDI_OK || count < 3 || count > TRACKS) {
        return GDI_INVALID;
    }
    at += strspn(at, " \t");
    if (*at != '\0') {
        return GDI_INVALID;
    }
    const char *slash = strrchr(image, '/');
    size_t folder = slash != NULL ? (size_t)(slash - image) + 1 : 0;
    while (disc->count < count) {
        error = gdi_line(gdi, line);
        if (error != GDI_OK) {
            return error;
        }
        const char *text = line + strspn(line, " \t");
        if (*text != '\0') {
            error = gdi_track(disc, text, image, folder);
            if (error != GDI_OK) {
                return error;
            }
        }
    }
    return GDI_OK;
}

static int gdi_image(struct gdi *disc, FILE *gdi, const char *image)
{
    // resolve .gdi symlinks before locating sibling track files
    char *real = realpath(image, NULL);
    int error = gdi_lines(disc, gdi, real != NULL ? real : image);
    free(real);
    return error;
}

static int volume(struct gdi *disc)
{
    // return at the first primary descriptor in sectors 16..31, refusing a
    // terminator before it. The rebuild survey checks the rest of the
    // descriptor sequence.
    if (disc->count < 3 || disc->tracks[2].start != HIGH ||
        !disc->tracks[2].data) {
        return GDI_INVALID;
    }
    unsigned char block[BLOCK];
    for (uint32_t at = HIGH + 16; at < HIGH + 32; at++) {
        int error = sector(disc, at, block);
        if (error != GDI_OK) {
            return error;
        }
        if (memcmp(block + 1, "CD001", 5) != 0 || block[6] != 1 ||
            block[0] == 255) {
            return GDI_INVALID;
        }
        if (block[0] == 1) {
            const unsigned char *root = block + 156;
            unsigned block_size = block[128] | (unsigned)block[129] << 8;
            if (block_size != BLOCK || root[0] != 34 || root[1] != 0 ||
                (root[25] & 0x82U) != 2 || root[26] != 0 || root[27] != 0 ||
                root[32] != 1 || root[33] != 0) {
                return GDI_INVALID;
            }
            disc->root.sector = le32(root + 2);
            disc->root.size = le32(root + 10);
            disc->root.directory = true;
            return GDI_OK;
        }
    }
    return GDI_INVALID;
}

int gdi_open(struct gdi **disc, const char *image)
{
    // keep the original .gdi open for byte-for-byte rebuilding.
    // Preserve track-specific blame if initialization fails.
    *disc = calloc(1, sizeof **disc);
    if (*disc == NULL) {
        return GDI_NOMEM;
    }
    size_t length = strlen(image);
    memcpy((*disc)->image, image, length < PATH ? length : PATH - 1);
    struct stat st;
    int error = open_file(image, &(*disc)->file, &st);
    if (error == GDI_OK) {
        error = gdi_image(*disc, (*disc)->file, image);
    }
    if (error == GDI_OK) {
        error = volume(*disc);
    }
    if (error != GDI_OK && error != GDI_NOMEM && (*disc)->failed[0] == '\0') {
        (void)fail(*disc, error, (*disc)->image);
    }
    return error;
}

static int name(struct entry *entry, const unsigned char *text, size_t bytes)
{
    // strip the ISO9660 version and a trailing dot on extensionless files.
    // Reject names that could escape the output directory.
    size_t size = 0;
    while (size < bytes && text[size] != ';') {
        size++;
    }
    if (!entry->directory && size != 0 && text[size - 1] == '.') {
        size--;
    }
    memcpy(entry->name, text, size);
    entry->name[size] = '\0';
    if (size == 0 || strlen(entry->name) != size ||
        strchr(entry->name, '/') != NULL || strcmp(entry->name, ".") == 0 ||
        strcmp(entry->name, "..") == 0) {
        return GDI_INVALID;
    }
    return GDI_OK;
}

static int begin(struct gdi *disc, struct cursor *cursor,
                 const struct entry *directory)
{
    cursor->sector = directory->sector;
    cursor->left = directory->size;
    cursor->at = 0;
    return directory->size != 0 ? sector(disc, cursor->sector, cursor->block)
                                : GDI_OK;
}

static int record_lengths(const unsigned char *record, size_t room,
                          size_t *length, size_t *bytes)
{
    // byte 32, the name's length, can't be read until the record has 34
    *length = record[0];
    if (*length < 34 || *length > room) {
        return GDI_INVALID;
    }
    *bytes = record[32];
    return 33 + *bytes <= *length ? GDI_OK : GDI_INVALID;
}

static int next(struct gdi *disc, struct cursor *cursor, struct entry *entry)
{
    // records never cross sectors. A zero length skips to the next block;
    // an empty name signals the end of the directory.
    for (;;) {
        size_t end = cursor->left < BLOCK ? cursor->left : BLOCK;
        if (cursor->at >= end || cursor->block[cursor->at] == 0) {
            if (cursor->left <= BLOCK) {
                entry->name[0] = '\0';
                return GDI_OK;
            }
            cursor->left -= BLOCK;
            cursor->sector++;
            cursor->at = 0;
            int error = sector(disc, cursor->sector, cursor->block);
            if (error != GDI_OK) {
                return error;
            }
            continue;
        }
        const unsigned char *record = cursor->block + cursor->at;
        size_t length;
        size_t bytes;
        if (record_lengths(record, end - cursor->at, &length, &bytes) !=
            GDI_OK) {
            return GDI_INVALID;
        }
        cursor->at += length;
        if ((bytes == 1 && record[33] <= 1) || (record[25] & 4U) != 0) {
            continue; // skip . / .. and associated files sharing a file's name
        }
        // these features change extent interpretation and are unsupported
        if (record[1] != 0 || record[26] != 0 || record[27] != 0 ||
            (record[25] & 0x80U) != 0) {
            return GDI_INVALID;
        }
        entry->sector = le32(record + 2);
        entry->size = le32(record + 10);
        entry->directory = (record[25] & 2U) != 0;
        return name(entry, record + 33, bytes);
    }
}

static int add(struct gdi *disc, struct walk *walker, size_t *length,
               const char *name, size_t size)
{
    // an overlong disc path is invalid when listing, but an output error
    // when extracting
    if (*length + 1 + size >= PATH) {
        errno = ENAMETOOLONG;
        return walker->extract ? fail(disc, GDI_WRITE, walker->path)
                               : fail(disc, GDI_INVALID, disc->image);
    }
    walker->path[(*length)++] = '/';
    memcpy(walker->path + *length, name, size);
    *length += size;
    walker->path[*length] = '\0';
    return GDI_OK;
}

static int find(struct gdi *disc, const char *path, struct entry *entry,
                struct walk *walker, size_t *length)
{
    // resolve components from the root; repeated slashes are ignored
    const char *wanted = path;
    *entry = disc->root;
    for (;;) {
        path += strspn(path, "/");
        size_t size = strcspn(path, "/");
        if (size == 0) {
            return GDI_OK;
        }
        if (!entry->directory) {
            return fail(disc, GDI_MISSING, wanted);
        }
        struct cursor cursor;
        struct entry child;
        int error = begin(disc, &cursor, entry);
        if (error != GDI_OK) {
            return error;
        }
        do {
            error = next(disc, &cursor, &child);
        } while (error == GDI_OK && child.name[0] != '\0' &&
                 (strlen(child.name) != size ||
                  memcmp(child.name, path, size) != 0));
        if (error != GDI_OK) {
            return error;
        }
        if (child.name[0] == '\0') {
            return fail(disc, GDI_MISSING, wanted);
        }
        error = add(disc, walker, length, path, size);
        if (error != GDI_OK) {
            return error;
        }
        *entry = child;
        path += size;
    }
}

static int copy(struct gdi *disc, const struct entry *entry, const char *path)
{
    // create exclusively; I/O failure can leave a partial file
    FILE *file = NULL;
    if (create(path, &file) != GDI_OK) {
        return fail(disc, GDI_WRITE, path);
    }
    unsigned char block[BLOCK];
    int error = GDI_OK;
    uint32_t at = entry->sector;
    for (uint32_t left = entry->size; left != 0 && error == GDI_OK; at++) {
        size_t size = left < BLOCK ? left : BLOCK;
        error = sector(disc, at, block);
        if (error == GDI_OK && fwrite(block, 1, size, file) != size) {
            error = fail(disc, GDI_WRITE, path);
        }
        left -= (uint32_t)size;
    }
    int saved = errno;
    if (fclose(file) != 0 && error == GDI_OK) {
        return fail(disc, GDI_WRITE, path);
    }
    errno = saved;
    return error;
}

static int make_directory(struct gdi *disc, const char *path)
{
    if (mkdir(path, 0777) == 0) {
        return GDI_OK;
    }
    struct stat st;
    if (errno != EEXIST || stat(path, &st) != 0) {
        return fail(disc, GDI_WRITE, path);
    }
    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return fail(disc, GDI_WRITE, path);
    }
    return GDI_OK;
}

// DEPTH bounds recursive directory traversal
// NOLINTNEXTLINE(misc-no-recursion)
static int walk(struct gdi *disc, struct walk *walker,
                const struct entry *entry, size_t length, unsigned depth)
{
    // visit each directory extent once; shared extents and cycles are
    // treated as damage
    if (!entry->directory) {
        if (walker->extract) {
            return copy(disc, entry, walker->path);
        }
        if (walker->each != NULL) {
            walker->each(walker->context, walker->path + 1, entry->size);
        }
        return GDI_OK;
    }
    uint32_t bit = entry->sector;
    if (depth == DEPTH || bit >= SECTORS ||
        (disc->seen[bit / 8] & 1U << (bit % 8)) != 0) {
        return GDI_INVALID;
    }
    disc->seen[bit / 8] |= (unsigned char)(1U << (bit % 8));
    if (walker->extract) {
        int error = make_directory(disc, walker->path);
        if (error != GDI_OK) {
            return error;
        }
    }
    struct cursor cursor;
    int error = begin(disc, &cursor, entry);
    while (error == GDI_OK) {
        struct entry child;
        error = next(disc, &cursor, &child);
        if (error != GDI_OK || child.name[0] == '\0') {
            break;
        }
        size_t longer = length;
        error = add(disc, walker, &longer, child.name, strlen(child.name));
        if (error == GDI_OK) {
            error = walk(disc, walker, &child, longer, depth + 1);
        }
        walker->path[length] = '\0';
    }
    return error;
}

int gdi_list(struct gdi *disc, const char *path,
             void (*each)(void *context, const char *path, uint32_t size),
             void *context)
{
    // internal paths have a leading slash; callbacks receive relative paths
    struct walk list = {.each = each, .context = context};
    struct entry entry;
    size_t length = 0;
    disc->failed[0] = '\0';
    int error = find(disc, path, &entry, &list, &length);
    if (error == GDI_OK) {
        memset(disc->seen, 0, sizeof disc->seen);
        error = walk(disc, &list, &entry, length, 0);
    }
    if (error != GDI_OK && disc->failed[0] == '\0') {
        (void)fail(disc, error, disc->image);
    }
    return error;
}

int gdi_extract(struct gdi *disc, const char *path, const char *directory)
{
    // create missing parents before walking the requested path
    struct walk out = {.extract = true};
    struct entry entry;
    disc->failed[0] = '\0';
    if (*directory == '\0') {
        directory = ".";
    }
    size_t length = strlen(directory);
    if (length >= PATH) {
        errno = ENAMETOOLONG;
        return fail(disc, GDI_WRITE, directory);
    }
    memcpy(out.path, directory, length + 1);
    int error = find(disc, path, &entry, &out, &length);
    for (size_t i = 1; i < length && error == GDI_OK; i++) {
        if (out.path[i] == '/' && out.path[i - 1] != '/') {
            out.path[i] = '\0';
            error = make_directory(disc, out.path);
            out.path[i] = '/';
        }
    }
    if (error == GDI_OK) {
        memset(disc->seen, 0, sizeof disc->seen);
        error = walk(disc, &out, &entry, length, 0);
    }
    if (error != GDI_OK && disc->failed[0] == '\0') {
        (void)fail(disc, error, disc->image);
    }
    return error;
}

// Rebuild surveys ISO9660 extents into a sector-ownership map. Edits change
// payloads and placements; a second map rejects overlaps before writing.
// Unchanged bytes are copied, modified raw sectors are resealed, and the
// staged image is verified before its .gdi receives the final name.

// the map's first parts: sectors in no other part, in an audio track or
// between tracks
enum { UNCLAIMED, AUDIO, GAP, FIRST };

// A stat fingerprint, not a snapshot of file contents.
struct stamp {
    dev_t device;
    ino_t inode;
    off_t size;
    struct timespec time; // ctime rather than user-settable mtime
};

// A surveyed extent and, for a file, its plan. start, count and size
// describe the original extent, in LBAs, sectors and bytes; new_start and
// new_size the planned one. record is the absolute LBA of the directory
// sector holding a file's record, and record_offset the record's byte
// offset within it. from is the opened replacement, or NULL; source is its
// path as the edit gives it, and stamp its state when opened.
struct part {
    int kind;
    uint32_t start; // for an empty file, the extent its record names
    uint32_t count;
    uint32_t size;
    char *path; // disc path of a file or directory, else NULL
    uint32_t record;
    unsigned record_offset;
    uint32_t new_start;
    uint32_t new_size;
    FILE *from;
    const char *source;
    struct stamp stamp;
    bool edited;  // an edit names it
    bool placed;  // the edit gives a start
    bool changed; // new bytes, start or size
};

// Survey and plan state, released after the operation. parts[root] is the
// root directory, and the parts from there on are named, sorted by path.
// boot is the boot file's part index, or 0, the unclaimed part, when no boot
// file is recognized. map[n - HIGH] is the index of the part that owns
// LBA n, for n below end.
struct rebuild {
    struct gdi *disc;
    FILE *files[TRACKS];             // every track, audio included
    uint32_t counts[TRACKS];         // each track's sectors
    struct stamp stamps[TRACKS + 1]; // the tracks', then the .gdi's
    struct part *parts;
    size_t count;
    size_t room;
    size_t root;
    size_t boot;
    uint32_t volume; // the volume's size as the primary descriptor gives it
    uint32_t end;    // the LBA after the volume's last
    uint32_t *map;
    uint32_t *records; // LBAs of directory sectors whose records change, sorted
    size_t record_count;
    unsigned char *buffer; // CHUNK bytes, for copying
};

// Path-table size and LBAs: little-endian, optional duplicate,
// big-endian, optional duplicate (0 when absent).
struct table {
    uint32_t size;
    uint32_t at[4];
};

static uint32_t be32(const unsigned char *at)
{
    return (uint32_t)at[0] << 24 | (uint32_t)at[1] << 16 |
           (uint32_t)at[2] << 8 | at[3];
}

static bool paired(const unsigned char *at)
{
    return le32(at) == be32(at + 4);
}

static void both(unsigned char *at, uint32_t value)
{
    for (unsigned i = 0; i < 4; i++) {
        at[i] = at[7 - i] = (unsigned char)(value >> (8 * i));
    }
}

static uint32_t blocks(uint32_t size)
{
    return (size / BLOCK) + (size % BLOCK != 0);
}

static const char *base(const char *path)
{
    const char *slash = strrchr(path, '/');
    return slash != NULL ? slash + 1 : path;
}

static int blame(struct rebuild *rb, size_t index, int error)
{
    // an error about a part: a file or directory, by its path on the disc,
    // and the root or a structure, by the .gdi
    const char *path = rb->parts[index].path;
    bool named = path != NULL && *path != '\0';
    return fail(rb->disc, error, named ? path : rb->disc->image);
}

static int stamp(FILE *file, struct stamp *now)
{
    // file's identity, size and status-change time now
    struct stat st;
    if (fstat(fileno(file), &st) != 0) {
        return GDI_READ;
    }
    now->device = st.st_dev;
    now->inode = st.st_ino;
    now->size = st.st_size;
    now->time = st.st_ctim;
    return GDI_OK;
}

static bool stamp_matches(FILE *file, const struct stamp *was)
{
    // whether file's size and status-change time are as they were
    struct stamp now;
    return stamp(file, &now) == GDI_OK && now.size == was->size &&
           now.time.tv_sec == was->time.tv_sec &&
           now.time.tv_nsec == was->time.tv_nsec;
}

static bool same_file(const struct stamp *one, const struct stamp *other)
{
    return one->device == other->device && one->inode == other->inode;
}

static int append(struct rebuild *rb, int kind, uint32_t start, uint32_t count,
                  size_t *index)
{
    // grow geometrically without overflowing the allocation size
    if (rb->count == rb->room) {
        if (rb->room > SIZE_MAX / sizeof *rb->parts / 2) {
            return GDI_NOMEM;
        }
        size_t room = rb->room != 0 ? rb->room * 2 : 64;
        struct part *grown = realloc(rb->parts, room * sizeof *grown);
        if (grown == NULL) {
            return GDI_NOMEM;
        }
        rb->parts = grown;
        rb->room = room;
    }
    struct part *part = &rb->parts[rb->count];
    memset(part, 0, sizeof *part);
    part->kind = kind;
    part->start = start;
    part->count = count;
    *index = rb->count++;
    return GDI_OK;
}

static int append_named(struct rebuild *rb, int kind, const char *folder,
                        const char *name, uint32_t start, uint32_t size,
                        size_t *index)
{
    // a part for a file or directory, whose path is folder/name, or name
    // when folder is the root's ""
    size_t at = *folder != '\0' ? strlen(folder) + 1 : 0;
    size_t length = strlen(name);
    char *path = malloc(at + length + 1);
    if (path == NULL) {
        return GDI_NOMEM;
    }
    if (at != 0) {
        memcpy(path, folder, at - 1);
        path[at - 1] = '/';
    }
    memcpy(path + at, name, length + 1);
    int error = append(rb, kind, start, blocks(size), index);
    if (error != GDI_OK) {
        free(path);
        return error;
    }
    struct part *part = &rb->parts[*index];
    part->path = path;
    part->size = size;
    part->new_start = start;
    part->new_size = size;
    return GDI_OK;
}

static int by_path(const void *left, const void *right)
{
    return strcmp(((const struct part *)left)->path,
                  ((const struct part *)right)->path);
}

static int by_number(const void *left, const void *right)
{
    uint32_t one = *(const uint32_t *)left;
    uint32_t other = *(const uint32_t *)right;
    return (one > other) - (one < other);
}

static int reread(struct rebuild *rb)
{
    // reparse the still-open .gdi and compare its track metadata.
    // Resolve paths beside the first track, independent of cwd.
    struct gdi *disc = rb->disc;
    struct gdi *again = calloc(1, sizeof *again);
    if (again == NULL) {
        return GDI_NOMEM;
    }
    int error = fseeko(disc->file, 0, SEEK_SET) == 0
                    ? gdi_lines(again, disc->file, disc->tracks[0].path)
                    : GDI_READ;
    if (error == GDI_INVALID) {
        error = GDI_CHANGED;
    }
    if (error != GDI_OK && again->failed[0] != '\0') {
        (void)fail(disc, error, again->failed);
    }
    if (error == GDI_OK && again->count != disc->count) {
        error = fail(disc, GDI_CHANGED, disc->image);
    }
    for (size_t i = 0; error == GDI_OK && i < disc->count; i++) {
        const struct track *was = &disc->tracks[i];
        const struct track *now = &again->tracks[i];
        if (now->start != was->start || now->offset != was->offset ||
            now->size != was->size || now->data != was->data ||
            strcmp(now->path, was->path) != 0) {
            error = fail(disc, GDI_CHANGED, disc->image);
        } else if (now->count != was->count) {
            error = fail(disc, GDI_CHANGED, was->path);
        }
    }
    gdi_close(again);
    return error;
}

static int track_files(struct rebuild *rb)
{
    // open audio tracks too. Require distinct regular files, ordered
    // non-overlapping extents, and no aliases to the .gdi.
    struct gdi *disc = rb->disc;
    int error = reread(rb);
    if (error != GDI_OK) {
        return error;
    }
    if (stamp(disc->file, &rb->stamps[disc->count]) != GDI_OK) {
        return fail(disc, GDI_READ, disc->image);
    }
    for (size_t i = 0; i < disc->count; i++) {
        const struct track *track = &disc->tracks[i];
        rb->files[i] = track->file;
        rb->counts[i] = track->count;
        if (track->file == NULL) {
            error = open_track(track, &rb->files[i], &rb->counts[i]);
        }
        if (error == GDI_OK) {
            error = stamp(rb->files[i], &rb->stamps[i]);
        }
        if (error == GDI_OK && i > 0 &&
            track->start < disc->tracks[i - 1].start + rb->counts[i - 1]) {
            error = GDI_INVALID;
        }
        for (size_t j = 0; error == GDI_OK && j < i; j++) {
            if (same_file(&rb->stamps[j], &rb->stamps[i])) {
                error = GDI_UNSUPPORTED;
            }
        }
        if (error == GDI_OK &&
            same_file(&rb->stamps[disc->count], &rb->stamps[i])) {
            error = GDI_UNSUPPORTED;
        }
        if (error != GDI_OK) {
            return fail(disc, error, track->path);
        }
    }
    return GDI_OK;
}

static int check_output_names(struct gdi *disc)
{
    // reserve the final and staged .gdi names. The staged-image
    // survey must still accept the temporary .part suffix.
    const char *name = base(disc->image);
    size_t length = strlen(name);
    for (size_t i = 0; i < disc->count; i++) {
        const char *own = base(disc->tracks[i].path);
        if (strcmp(own, name) == 0 || (strncmp(own, name, length) == 0 &&
                                       strcmp(own + length, ".part") == 0)) {
            return fail(disc, GDI_UNSUPPORTED, disc->tracks[i].path);
        }
    }
    return GDI_OK;
}

static int descriptors(struct rebuild *rb, unsigned char pvd[BLOCK])
{
    // unlike reading, rebuilding requires a terminator and exactly one
    // PVD. Other descriptors may refer to structures we cannot update.
    unsigned char block[BLOCK];
    bool primary = false;
    uint32_t at = HIGH + 16;
    for (;; at++) {
        int error = at < HIGH + 32 ? sector(rb->disc, at, block) : GDI_INVALID;
        if (error != GDI_OK) {
            return error;
        }
        if (memcmp(block + 1, "CD001", 5) != 0 || block[6] != 1) {
            return GDI_INVALID;
        }
        if (block[0] == 255) {
            break;
        }
        if (block[0] != 1 || primary) {
            return GDI_UNSUPPORTED;
        }
        memcpy(pvd, block, BLOCK);
        primary = true;
    }
    const unsigned char *root = pvd + 156;
    if (!primary || ((unsigned)pvd[130] << 8 | pvd[131]) != BLOCK ||
        !paired(pvd + 80) || !paired(pvd + 132) || !paired(root + 2) ||
        !paired(root + 10)) {
        return GDI_INVALID;
    }
    size_t index;
    int error = append(rb, GDI_SYSTEM_AREA, HIGH, 16, &index);
    if (error == GDI_OK) {
        error = append(rb, GDI_DESCRIPTORS, HIGH + 16, at + 1 - (HIGH + 16),
                       &index);
    }
    return error;
}

static int tables(struct rebuild *rb, const unsigned char pvd[BLOCK],
                  struct table *table)
{
    // reserve every path-table copy; the root entry is mandatory
    *table = (struct table){
        le32(pvd + 132),
        {le32(pvd + 140), le32(pvd + 144), be32(pvd + 148), be32(pvd + 152)}};
    uint32_t count = blocks(table->size);
    if (table->size < 10 || count >= SECTORS || table->at[0] == 0 ||
        table->at[2] == 0) {
        return GDI_INVALID;
    }
    for (size_t i = 0; i < 4; i++) {
        uint32_t at = table->at[i];
        size_t index;
        int error =
            at != 0 ? append(rb, GDI_PATH_TABLE, at, count, &index) : GDI_OK;
        if (error != GDI_OK) {
            return error;
        }
    }
    return GDI_OK;
}

// DEPTH bounds recursive directory traversal
// NOLINTNEXTLINE(misc-no-recursion)
static int directory(struct rebuild *rb, size_t self, uint32_t parent,
                     unsigned depth)
{
    // require . and .. first, matching this directory and its parent.
    // Save child record offsets for in-place extent updates; reject shared
    // directories and metadata the rebuilder cannot preserve.
    struct gdi *disc = rb->disc;
    uint32_t start = rb->parts[self].start;
    uint32_t size = rb->parts[self].size;
    if (depth == DEPTH || start >= SECTORS || size == 0 || size % BLOCK != 0 ||
        (disc->seen[start / 8] & 1U << (start % 8)) != 0) {
        return GDI_INVALID;
    }
    disc->seen[start / 8] |= (unsigned char)(1U << (start % 8));
    unsigned char block[BLOCK];
    size_t records = 0;
    for (uint32_t at = start; at - start < size / BLOCK; at++) {
        int error = sector(disc, at, block);
        if (error != GDI_OK) {
            return error;
        }
        for (size_t i = 0; i < BLOCK && block[i] != 0; i += block[i]) {
            const unsigned char *record = block + i;
            size_t length;
            size_t bytes;
            if (record_lengths(record, BLOCK - i, &length, &bytes) != GDI_OK ||
                !paired(record + 2) || !paired(record + 10)) {
                return GDI_INVALID;
            }
            uint32_t extent = le32(record + 2);
            uint32_t data = le32(record + 10);
            struct entry entry = {.directory = (record[25] & 2U) != 0};
            bool dots = bytes == 1 && record[33] <= 1;
            // these features change extent interpretation or reference
            // structures the rebuilder cannot update
            bool more = record[1] != 0 || record[26] != 0 || record[27] != 0 ||
                        (record[25] & 0x84U) != 0 ||
                        length > 33 + bytes + (bytes % 2 == 0);
            if (records < 2) {
                // . is the directory itself, and .. its parent
                if (!dots || !entry.directory || record[33] != records ||
                    extent != (records == 0 ? start : parent) ||
                    (records == 0 && data != size)) {
                    return GDI_INVALID;
                }
                if (more) {
                    return blame(rb, self, GDI_UNSUPPORTED);
                }
                records++;
                continue;
            }
            records++;
            if (dots || name(&entry, record + 33, bytes) != GDI_OK) {
                return GDI_INVALID;
            }
            size_t child;
            error = append_named(rb, entry.directory ? GDI_DIRECTORY : GDI_FILE,
                                 rb->parts[self].path, entry.name, extent, data,
                                 &child);
            if (error != GDI_OK) {
                return error;
            }
            if (more) {
                return blame(rb, child, GDI_UNSUPPORTED);
            }
            rb->parts[child].record = at;
            rb->parts[child].record_offset = (unsigned)i;
            error = entry.directory ? directory(rb, child, start, depth + 1)
                                    : GDI_OK;
            if (error != GDI_OK) {
                return error;
            }
        }
    }
    return records >= 2 ? GDI_OK : GDI_INVALID;
}

static int gather(struct rebuild *rb, uint32_t start, uint32_t size,
                  unsigned char *out)
{
    // size bytes, from the sectors from start
    unsigned char block[BLOCK];
    for (uint32_t done = 0; done < size; done += BLOCK) {
        int error = sector(rb->disc, start + (done / BLOCK), block);
        if (error != GDI_OK) {
            return error;
        }
        memcpy(out + done, block, size - done < BLOCK ? size - done : BLOCK);
    }
    return GDI_OK;
}

static bool under(const char *path, const char *folder)
{
    // whether folder is the directory path is in: path less its last name
    // and the slash before it, or for the root, the root
    size_t length = (size_t)(base(path) - path);
    size_t own = length != 0 ? length - 1 : 0;
    return strlen(folder) == own && strncmp(path, folder, own) == 0;
}

static int check_tables(struct rebuild *rb, const struct table *table)
{
    // copies must agree and enumerate every directory exactly once,
    // under the correct parent, without extended attributes
    uint32_t size = table->size;
    size_t directories = 1; // the root, then those after it
    for (size_t j = rb->root + 1; j < rb->count; j++) {
        directories += rb->parts[j].kind == GDI_DIRECTORY;
    }
    unsigned char *copies[4] = {NULL, NULL, NULL, NULL};
    size_t *entries = malloc(directories * sizeof *entries);
    size_t *parents = malloc(directories * sizeof *parents);
    bool *found = calloc(rb->count, sizeof *found);
    int error = entries != NULL && parents != NULL && found != NULL ? GDI_OK
                                                                    : GDI_NOMEM;
    for (size_t i = 0; error == GDI_OK && i < 4; i++) {
        uint32_t at = table->at[i];
        if (at != 0) {
            copies[i] = malloc(size);
            error =
                copies[i] != NULL ? gather(rb, at, size, copies[i]) : GDI_NOMEM;
        }
    }
    const unsigned char *little = copies[0];
    const unsigned char *big = copies[2];
    if (error == GDI_OK &&
        ((copies[1] != NULL && memcmp(copies[1], little, size) != 0) ||
         (copies[3] != NULL && memcmp(copies[3], big, size) != 0))) {
        error = GDI_INVALID;
    }
    size_t count = 0;
    for (size_t i = 0; error == GDI_OK && i < size;) {
        size_t length = little[i];
        if (length == 0 || 8 + length > size - i || big[i] != length ||
            little[i + 1] != big[i + 1] ||
            le32(little + i + 2) != be32(big + i + 2) ||
            little[i + 6] != big[i + 7] || little[i + 7] != big[i + 6] ||
            memcmp(little + i + 8, big + i + 8, length) != 0) {
            error = GDI_INVALID;
            break;
        }
        if (little[i + 1] != 0) {
            error = GDI_UNSUPPORTED;
            break;
        }
        size_t named = 0;
        for (size_t j = rb->root; j < rb->count && named == 0; j++) {
            if (rb->parts[j].kind == GDI_DIRECTORY &&
                rb->parts[j].start == le32(little + i + 2)) {
                named = j;
            }
        }
        const char *path = named != 0 ? rb->parts[named].path : "";
        bool root = *path == '\0';
        if (named == 0 || found[named] ||
            (root ? length != 1 || little[i + 8] != 0
                  : strlen(base(path)) != length ||
                        memcmp(base(path), little + i + 8, length) != 0)) {
            error = GDI_INVALID;
            break;
        }
        found[named] = true;
        entries[count] = named;
        parents[count++] = (size_t)little[i + 6] | (size_t)little[i + 7] << 8;
        i += 8 + length + (length % 2);
    }
    if (error == GDI_OK && count != directories) {
        error = GDI_INVALID;
    }
    for (size_t j = 0; error == GDI_OK && j < count; j++) {
        if (parents[j] == 0 || parents[j] > count ||
            !under(rb->parts[entries[j]].path,
                   rb->parts[entries[parents[j] - 1]].path)) {
            error = GDI_INVALID;
        }
    }
    for (size_t i = 0; i < 4; i++) {
        free(copies[i]);
    }
    free(entries);
    free(parents);
    free(found);
    return error;
}

static const struct track *last_data_track(const struct gdi *disc)
{
    // track 3, which the volume starts in, or a later one
    const struct track *last = &disc->tracks[2];
    for (size_t i = 3; i < disc->count; i++) {
        if (disc->tracks[i].data) {
            last = &disc->tracks[i];
        }
    }
    return last;
}

static uint32_t span(const struct part *part, bool planned, uint32_t *start)
{
    // how many sectors a part holds, and from where: on the disc or, once
    // planned, where a file's will be
    bool moves = planned && part->kind == GDI_FILE;
    *start = moves ? part->new_start : part->start;
    return moves ? blocks(part->new_size) : part->count;
}

static int resolve_volume_end(struct rebuild *rb, bool planned)
{
    // PVD volume size is absolute on some discs, relative to LBA 45000
    // on others. Exactly one interpretation must cover all extents and
    // stay within the final data track, before and after planning.
    uint64_t used = HIGH;
    for (size_t i = FIRST; i < rb->count; i++) {
        uint32_t start;
        uint32_t count = span(&rb->parts[i], planned, &start);
        if (count != 0 && start + (uint64_t)count > used) {
            used = start + (uint64_t)count;
        }
    }
    const struct track *track = last_data_track(rb->disc);
    uint64_t last = track->start + (uint64_t)track->count;
    uint64_t size = rb->volume;
    bool from_disc = size >= used && size <= last;
    bool from_area = HIGH + size >= used && HIGH + size <= last;
    if (from_disc == from_area) {
        return GDI_UNSUPPORTED;
    }
    rb->end = (uint32_t)(from_disc ? size : HIGH + size);
    return GDI_OK;
}

static int fill(struct rebuild *rb, size_t index, bool planned)
{
    // reject out-of-volume, audio and gap sectors, and conflicting owners.
    // Collisions caused by explicit placement return GDI_OVERLAP.
    const struct part *part = &rb->parts[index];
    uint32_t start;
    uint32_t count = span(part, planned, &start);
    for (uint64_t at = start; at < start + (uint64_t)count; at++) {
        uint32_t *owner =
            at >= HIGH && at < rb->end ? &rb->map[at - HIGH] : NULL;
        if (owner == NULL || *owner == AUDIO || *owner == GAP) {
            return blame(rb, index, part->placed ? GDI_OVERLAP : GDI_INVALID);
        }
        if (*owner != UNCLAIMED) {
            return blame(rb, index,
                         part->placed ? GDI_OVERLAP : GDI_UNSUPPORTED);
        }
        *owner = (uint32_t)index;
    }
    return GDI_OK;
}

static int map_ownership(struct rebuild *rb, bool planned)
{
    // seed track ownership, then claim filesystem extents. Placed files
    // come last so an overlap is attributed to the edit.
    size_t length = rb->end - HIGH;
    if (rb->map == NULL) {
        rb->map = calloc(length, sizeof *rb->map);
        if (rb->map == NULL) {
            return GDI_NOMEM;
        }
    }
    for (size_t i = 0; i < length; i++) {
        rb->map[i] = GAP;
    }
    for (size_t i = 0; i < rb->disc->count; i++) {
        const struct track *track = &rb->disc->tracks[i];
        uint64_t end = track->start + (uint64_t)rb->counts[i];
        for (uint64_t at = track->start > HIGH ? track->start : HIGH;
             at < end && at < rb->end; at++) {
            rb->map[at - HIGH] = track->data ? UNCLAIMED : AUDIO;
        }
    }
    for (int round = 0; round < 2; round++) {
        for (size_t i = FIRST; i < rb->count; i++) {
            int error = rb->parts[i].placed == (round == 1)
                            ? fill(rb, i, planned)
                            : GDI_OK;
            if (error != GDI_OK) {
                return error;
            }
        }
    }
    return GDI_OK;
}

static int find_boot(struct rebuild *rb)
{
    // recognize a Katana signature, space-padded boot filename, and final
    // peripheral nibble with the Windows CE bit clear. The bootstrap's
    // instructions are not inspected.
    static const char digits[] = "0123456789ABCDEF";
    unsigned char ip[BLOCK];
    int error = sector(rb->disc, HIGH, ip);
    if (error != GDI_OK) {
        return error;
    }
    const unsigned char *boot = ip + 0x60;
    const char *digit = ip[0x3e] != 0 ? strchr(digits, ip[0x3e]) : NULL;
    char name[17];
    size_t length = 0;
    while (length < 16 && boot[length] > ' ' && boot[length] < 0x7f &&
           boot[length] != '/') {
        name[length] = (char)boot[length];
        length++;
    }
    name[length] = '\0';
    bool padded = length != 0;
    for (size_t i = length; i < 16; i++) {
        padded = padded && boot[i] == ' ';
    }
    if (memcmp(ip, "SEGA SEGAKATANA ", 16) != 0 || !padded || digit == NULL ||
        (digit - digits) % 2 != 0) {
        return GDI_OK;
    }
    struct part key = {.path = name};
    const struct part *found = bsearch(
        &key, rb->parts + rb->root, rb->count - rb->root, sizeof key, by_path);
    if (found != NULL && found->kind == GDI_FILE && found->size != 0) {
        rb->boot = (size_t)(found - rb->parts);
    }
    return GDI_OK;
}

static int survey(struct rebuild *rb)
{
    // the disc as a rebuild needs it: its tracks' files, every part of its
    // volume, each name once, and the volume's end, its map and the boot
    // file
    static const int kinds[FIRST] = {
        [UNCLAIMED] = GDI_UNREFERENCED, [AUDIO] = GDI_AUDIO, [GAP] = GDI_GAP};
    unsigned char pvd[BLOCK] = {0};
    struct table table;
    size_t index;
    int error = track_files(rb);
    for (size_t i = 0; error == GDI_OK && i < FIRST; i++) {
        error = append(rb, kinds[i], 0, 0, &index);
    }
    if (error == GDI_OK) {
        error = descriptors(rb, pvd);
    }
    if (error == GDI_OK) {
        error = tables(rb, pvd, &table);
    }
    if (error == GDI_OK) {
        rb->root = rb->count;
        error = append_named(rb, GDI_DIRECTORY, "", "", le32(pvd + 158),
                             le32(pvd + 166), &index);
    }
    if (error == GDI_OK) {
        memset(rb->disc->seen, 0, sizeof rb->disc->seen);
        error = directory(rb, rb->root, rb->parts[rb->root].start, 0);
    }
    if (error == GDI_OK) {
        // ISO9660 version suffixes are stripped, so a duplicate path is
        // ambiguous
        qsort(rb->parts + rb->root, rb->count - rb->root, sizeof *rb->parts,
              by_path);
        for (size_t i = rb->root + 1; i < rb->count && error == GDI_OK; i++) {
            if (strcmp(rb->parts[i - 1].path, rb->parts[i].path) == 0) {
                error = blame(rb, i, GDI_UNSUPPORTED);
            }
        }
    }
    if (error == GDI_OK) {
        error = check_tables(rb, &table);
    }
    if (error == GDI_OK) {
        rb->volume = le32(pvd + 80);
        error = resolve_volume_end(rb, false);
    }
    if (error == GDI_OK) {
        error = map_ownership(rb, false);
    }
    if (error == GDI_OK) {
        error = find_boot(rb);
    }
    return error;
}

static int find_file(struct rebuild *rb, const char *path, size_t *index)
{
    // the file at path, which can have slashes to spare, as gdi_list's can.
    // A directory isn't a file a rebuild changes.
    char wanted[PATH];
    size_t length = 0;
    for (const char *at = path + strspn(path, "/"); *at != '\0';
         at += strspn(at, "/")) {
        size_t size = strcspn(at, "/");
        if (length + 1 + size >= PATH) {
            return fail(rb->disc, GDI_MISSING, path);
        }
        if (length != 0) {
            wanted[length++] = '/';
        }
        memcpy(wanted + length, at, size);
        length += size;
        at += size;
    }
    wanted[length] = '\0';
    struct part key = {.path = wanted};
    const struct part *found = bsearch(
        &key, rb->parts + rb->root, rb->count - rb->root, sizeof key, by_path);
    if (found == NULL) {
        return fail(rb->disc, GDI_MISSING, path);
    }
    if (found->kind != GDI_FILE) {
        return fail(rb->disc, GDI_UNSUPPORTED, path);
    }
    *index = (size_t)(found - rb->parts);
    return GDI_OK;
}

static int replace(struct rebuild *rb, size_t index, const char *from)
{
    // file index's new bytes, from a regular file opened and measured now,
    // which a record's 32-bit size must hold. The boot file can't be emptied.
    struct part *file = &rb->parts[index];
    struct stat st;
    int error = open_file(from, &file->from, &st);
    if (error == GDI_INVALID) {
        errno = S_ISDIR(st.st_mode) ? EISDIR : EINVAL;
        error = GDI_READ;
    }
    if (error == GDI_OK) {
        error = stamp(file->from, &file->stamp);
    }
    if (error != GDI_OK) {
        return fail(rb->disc, error, from);
    }
    file->source = from;
    if ((uint64_t)st.st_size > UINT32_MAX ||
        (index == rb->boot && st.st_size == 0)) {
        return blame(rb, index, GDI_UNSUPPORTED);
    }
    file->new_size = (uint32_t)st.st_size;
    return GDI_OK;
}

static int check_place(struct rebuild *rb, size_t index)
{
    // growth beyond the original extent needs placement. The boot file
    // keeps its exclusive end; moving it requires both starts at LATE or
    // later and the new start in the final data track. Booting is not
    // guaranteed, and the ownership map still rejects collisions.
    const struct part *file = &rb->parts[index];
    uint32_t count = blocks(file->new_size);
    if (index == rb->boot) {
        uint64_t end = file->start + (uint64_t)file->count;
        bool moves = count != file->count;
        bool late = file->start >= LATE && file->new_start >= LATE &&
                    file->new_start >= last_data_track(rb->disc)->start;
        return file->new_start + (uint64_t)count == end && (!moves || late)
                   ? GDI_OK
                   : blame(rb, index, GDI_UNSUPPORTED);
    }
    if (file->placed && count == 0) {
        return blame(rb, index, GDI_UNSUPPORTED);
    }
    if (!file->placed && count > file->count) {
        return blame(rb, index, GDI_NEEDS_PLACE);
    }
    return GDI_OK;
}

static int apply_edits(struct rebuild *rb, const struct gdi_edit *edits,
                       size_t count)
{
    // the files the edits name, each only once, with their new bytes, and
    // where each starts: where it did, unless it's placed
    for (size_t i = 0; i < count; i++) {
        const struct gdi_edit *edit = &edits[i];
        size_t index = 0;
        int error = find_file(rb, edit->path, &index);
        if (error != GDI_OK) {
            return error;
        }
        struct part *file = &rb->parts[index];
        if (file->edited) {
            return fail(rb->disc, GDI_DUPLICATE, edit->path);
        }
        file->edited = true;
        file->placed = edit->start != 0;
        file->new_start = file->placed ? edit->start : file->start;
        if (edit->from != NULL) {
            error = replace(rb, index, edit->from);
        }
        if (error == GDI_OK) {
            error = check_place(rb, index);
        }
        if (error != GDI_OK) {
            return error;
        }
    }
    return GDI_OK;
}

static int payload(struct rebuild *rb, size_t index, uint32_t sector_index,
                   unsigned char data[BLOCK])
{
    // read from the replacement or the original extent, zeros past the end
    const struct part *file = &rb->parts[index];
    uint64_t offset = (uint64_t)sector_index * BLOCK;
    size_t length = file->new_size - offset < BLOCK
                        ? (size_t)(file->new_size - offset)
                        : BLOCK;
    int error = GDI_OK;
    if (file->from == NULL) {
        error = sector(rb->disc, file->start + sector_index, data);
    } else if (fseeko(file->from, (off_t)offset, SEEK_SET) != 0) {
        error = fail(rb->disc, GDI_READ, file->source);
    } else if (fread(data, 1, length, file->from) != length) {
        error = fail(rb->disc, ferror(file->from) != 0 ? GDI_READ : GDI_CHANGED,
                     file->source);
    }
    memset(data + length, 0, BLOCK - length);
    return error;
}

static int differs(struct rebuild *rb, size_t index, bool *differ)
{
    // whether the new bytes differ from the old within the old size. The
    // padding past the file's end isn't compared, so an identical
    // replacement keeps its sectors as they were, raw error codes included.
    const struct part *file = &rb->parts[index];
    unsigned char was[BLOCK];
    unsigned char now[BLOCK];
    *differ = false;
    for (uint32_t i = 0; i < file->count && !*differ; i++) {
        int error = payload(rb, index, i, now);
        if (error == GDI_OK) {
            error = sector(rb->disc, file->start + i, was);
        }
        if (error != GDI_OK) {
            return error;
        }
        uint64_t left = file->size - ((uint64_t)i * BLOCK);
        *differ = memcmp(was, now, left < BLOCK ? (size_t)left : BLOCK) != 0;
    }
    return GDI_OK;
}

static int writable(struct rebuild *rb, uint32_t number)
{
    // whether a rebuild can write the sector with that number: cooked, or
    // raw in mode 1, whose header it keeps. Mode 2 has a subheader, and
    // error codes of its own.
    const struct track *track = locate(rb->disc, number);
    if (track == NULL) {
        return GDI_INVALID;
    }
    if (!track->raw) {
        return GDI_OK;
    }
    unsigned char in[RAW];
    int error = fetch(track, number - track->start, in);
    if (error != GDI_OK) {
        return fail(rb->disc, error, track->path);
    }
    if (!sector_header_matches(in, number)) {
        return fail(rb->disc, GDI_INVALID, track->path);
    }
    return in[15] == 1 ? GDI_OK : GDI_UNSUPPORTED;
}

static int check_file(struct rebuild *rb, size_t index)
{
    // a changed file's new sectors must be writable, and so must its
    // record's sector if it moves or resizes; that sector joins rb->records.
    // gdi has to recognize the boot file.
    const struct part *file = &rb->parts[index];
    uint64_t end = file->new_start + (uint64_t)blocks(file->new_size);
    if (rb->boot == 0) {
        return fail(rb->disc, GDI_UNSUPPORTED, rb->disc->image);
    }
    for (uint64_t at = file->new_start; at < end; at++) {
        int error = writable(rb, (uint32_t)at);
        if (error != GDI_OK) {
            return error == GDI_UNSUPPORTED ? blame(rb, index, error) : error;
        }
    }
    if (file->new_start != file->start || file->new_size != file->size) {
        int error = writable(rb, file->record);
        if (error != GDI_OK) {
            return error == GDI_UNSUPPORTED ? blame(rb, index, error) : error;
        }
        rb->records[rb->record_count++] = file->record;
    }
    return GDI_OK;
}

static int validate_plan(struct rebuild *rb)
{
    // validate final ownership and volume bounds, then mark the files that
    // change and collect the directory sectors whose records change
    int error = map_ownership(rb, true);
    if (error == GDI_OK) {
        error = resolve_volume_end(rb, true);
    }
    if (error != GDI_OK) {
        return error;
    }
    rb->records = calloc(rb->count - rb->root, sizeof *rb->records);
    if (rb->records == NULL) {
        return GDI_NOMEM;
    }
    for (size_t i = rb->root; i < rb->count; i++) {
        struct part *file = &rb->parts[i];
        if (file->kind != GDI_FILE) {
            continue;
        }
        file->changed =
            file->new_start != file->start || file->new_size != file->size;
        if (!file->changed && file->from != NULL) {
            error = differs(rb, i, &file->changed);
        }
        if (error == GDI_OK && file->changed) {
            error = check_file(rb, i);
        }
        if (error != GDI_OK) {
            return error;
        }
    }
    qsort(rb->records, rb->record_count, sizeof *rb->records, by_number);
    return GDI_OK;
}

static unsigned twice(unsigned value)
{
    // GF(256) multiplication by x, modulo ECMA-130's polynomial 0x11d
    return (value << 1 ^ ((value & 0x80U) != 0 ? 0x11dU : 0)) & 0xffU;
}

static unsigned times(unsigned value, unsigned by)
{
    unsigned product = 0;
    for (; by != 0; by >>= 1) {
        if ((by & 1U) != 0) {
            product ^= value;
        }
        value = twice(value);
    }
    return product;
}

static void parity(const unsigned char raw[RAW], unsigned vectors,
                   unsigned length, unsigned across, unsigned step,
                   unsigned char *out)
{
    // ECMA-130 P/Q parity: two bytes per interleaved vector make its XOR
    // sum and x-weighted sum zero. across/step select columns or diagonals;
    // outputs group first parity bytes before second. 0xf4 = 1 / (x + 1).
    unsigned size = vectors * length;
    for (unsigned vector = 0; vector < vectors; vector++) {
        unsigned sum = 0;
        unsigned weighted = 0;
        unsigned at = (vector / 2 * across) + (vector % 2);
        for (unsigned i = 0; i < length; i++) {
            sum ^= raw[12 + at];
            weighted = twice(weighted ^ raw[12 + at]);
            at = (at + step) % size;
        }
        unsigned first = times(twice(weighted) ^ sum, 0xf4);
        out[vector] = (unsigned char)first;
        out[vector + vectors] = (unsigned char)(first ^ sum);
    }
}

static void seal(unsigned char raw[RAW])
{
    // mode 1's EDC covers sync, header and data (reflected polynomial
    // 0xd8018001); eight reserved bytes follow. ECMA-130 P parity covers
    // 86 columns and Q parity 52 diagonals, starting at the header.
    uint32_t edc = 0;
    for (size_t i = 0; i < 16 + BLOCK; i++) {
        edc ^= raw[i];
        for (int bit = 0; bit < 8; bit++) {
            edc = edc >> 1 ^ ((edc & 1U) != 0 ? 0xd8018001U : 0);
        }
    }
    for (unsigned i = 0; i < 4; i++) {
        raw[16 + BLOCK + i] = (unsigned char)(edc >> (8 * i));
    }
    memset(raw + 16 + BLOCK + 4, 0, 8);
    parity(raw, 86, 24, 2, 86, raw + 2076);
    parity(raw, 52, 43, 86, 88, raw + 2248);
}

static void rewrite(const struct rebuild *rb, uint32_t number,
                    unsigned char data[BLOCK])
{
    // the records in the sector with that number of the files that change,
    // with their extents and sizes after the rebuild
    for (size_t i = rb->root; i < rb->count; i++) {
        const struct part *file = &rb->parts[i];
        if (file->kind == GDI_FILE && file->changed && file->record == number) {
            both(data + file->record_offset + 2, file->new_start);
            both(data + file->record_offset + 10, file->new_size);
        }
    }
}

static bool changes(const struct rebuild *rb, uint32_t number)
{
    // whether the plan changes the sector with that number: a changed
    // file's, or one with records that change
    if (number < HIGH || number >= rb->end) {
        return false;
    }
    const struct part *part = &rb->parts[rb->map[number - HIGH]];
    return (part->kind == GDI_FILE && part->changed) ||
           (rb->record_count != 0 &&
            bsearch(&number, rb->records, rb->record_count, sizeof number,
                    by_number) != NULL);
}

static int remake(struct rebuild *rb, uint32_t number, unsigned char raw[RAW])
{
    // the sector with that number, which writable accepted, as the rebuild
    // leaves it: just as it was if its data is the same, or else with its
    // new data and, if raw, new error codes under its own header
    const struct track *track = locate(rb->disc, number);
    if (track == NULL) {
        return GDI_INVALID;
    }
    int error = fetch(track, number - track->start, raw);
    if (error != GDI_OK) {
        // a short read, fetch's GDI_INVALID, means the track shrank
        return fail(rb->disc, error == GDI_INVALID ? GDI_CHANGED : error,
                    track->path);
    }
    unsigned char *data = track->raw ? raw + 16 : raw;
    unsigned char fresh[BLOCK];
    size_t owner = rb->map[number - HIGH];
    const struct part *file = &rb->parts[owner];
    if (file->kind == GDI_FILE && file->changed) {
        error = payload(rb, owner, number - file->new_start, fresh);
    } else {
        memcpy(fresh, data, BLOCK);
        rewrite(rb, number, fresh);
    }
    if (error == GDI_OK && memcmp(fresh, data, BLOCK) != 0) {
        memcpy(data, fresh, BLOCK);
        if (track->raw) {
            seal(raw);
        }
    }
    return error;
}

static int copy_bytes(struct rebuild *rb, size_t index, FILE *out,
                      const char *path, uint64_t from, uint64_t to)
{
    // track index's file's bytes in [from, to), as they are, out to path
    const struct track *track = &rb->disc->tracks[index];
    FILE *in = rb->files[index];
    if (from < to && fseeko(in, (off_t)from, SEEK_SET) != 0) {
        return fail(rb->disc, GDI_READ, track->path);
    }
    while (from < to) {
        size_t size = to - from < CHUNK ? (size_t)(to - from) : CHUNK;
        if (fread(rb->buffer, 1, size, in) != size) {
            return fail(rb->disc, ferror(in) != 0 ? GDI_READ : GDI_CHANGED,
                        track->path);
        }
        if (fwrite(rb->buffer, 1, size, out) != size) {
            return fail(rb->disc, GDI_WRITE, path);
        }
        from += size;
    }
    return GDI_OK;
}

static int finish(struct rebuild *rb, FILE *out, const char *path, int error)
{
    // a file the rebuild made at path, closed, and removed again if writing
    // it failed. errno stays as the first failure left it.
    int saved = errno;
    if (fclose(out) != 0 && error == GDI_OK) {
        error = fail(rb->disc, GDI_WRITE, path);
        saved = errno;
    }
    if (error != GDI_OK) {
        (void)unlink(path);
    }
    errno = saved;
    return error;
}

static int write_track(struct rebuild *rb, size_t index, const char *path)
{
    // track index's file, copied whole into a new one at path, with each
    // sector the plan changes made anew. A copy that fails is removed.
    const struct track *track = &rb->disc->tracks[index];
    uint32_t last = track->start + rb->counts[index];
    uint64_t length =
        track->offset + ((uint64_t)rb->counts[index] * track->size);
    uint64_t done = 0;
    FILE *out = NULL;
    if (create(path, &out) != GDI_OK) {
        return fail(rb->disc, GDI_WRITE, path);
    }
    int error = GDI_OK;
    for (uint32_t number = track->start; error == GDI_OK && number < last;
         number++) {
        if (!changes(rb, number)) {
            continue;
        }
        uint64_t at =
            track->offset + ((uint64_t)(number - track->start) * track->size);
        unsigned char raw[RAW];
        error = copy_bytes(rb, index, out, path, done, at);
        if (error == GDI_OK) {
            error = remake(rb, number, raw);
        }
        if (error == GDI_OK && fwrite(raw, track->size, 1, out) != 1) {
            error = fail(rb->disc, GDI_WRITE, path);
        }
        done = at + track->size;
    }
    if (error == GDI_OK) {
        error = copy_bytes(rb, index, out, path, done, length);
    }
    return finish(rb, out, path, error);
}

static int copy_gdi(struct rebuild *rb, const char *path)
{
    // the .gdi, byte for byte, from the file the disc keeps, into a new one
    // at path. A copy that fails is removed.
    FILE *in = rb->disc->file;
    FILE *out = NULL;
    if (create(path, &out) != GDI_OK) {
        return fail(rb->disc, GDI_WRITE, path);
    }
    int error = fseeko(in, 0, SEEK_SET) == 0
                    ? GDI_OK
                    : fail(rb->disc, GDI_READ, rb->disc->image);
    for (size_t size = CHUNK; error == GDI_OK && size == CHUNK;) {
        size = fread(rb->buffer, 1, CHUNK, in);
        if (size < CHUNK && ferror(in) != 0) {
            error = fail(rb->disc, GDI_READ, rb->disc->image);
        } else if (fwrite(rb->buffer, 1, size, out) != size) {
            error = fail(rb->disc, GDI_WRITE, path);
        }
    }
    return finish(rb, out, path, error);
}

static void release(struct rebuild *rb)
{
    for (size_t i = 0; i < rb->count; i++) {
        free(rb->parts[i].path);
        if (rb->parts[i].from != NULL) {
            (void)fclose(rb->parts[i].from);
        }
    }
    for (size_t i = 0; rb->disc != NULL && i < rb->disc->count; i++) {
        if (rb->files[i] != NULL && rb->files[i] != rb->disc->tracks[i].file) {
            (void)fclose(rb->files[i]);
        }
    }
    free(rb->parts);
    free(rb->map);
    free(rb->records);
    free(rb->buffer);
}

static int verify(struct rebuild *rb, const char *staged)
{
    // resurvey the staged image and compare the planned metadata, extents
    // and changed payloads. Unchanged sectors are not compared again.
    struct gdi *copy = NULL;
    int error = gdi_open(&copy, staged);
    struct rebuild again = {.disc = copy};
    if (error == GDI_OK) {
        error = survey(&again);
    }
    bool same = error == GDI_OK && again.count == rb->count &&
                again.end == rb->end && again.boot == rb->boot;
    for (size_t i = 0; same && i < rb->disc->count; i++) {
        const struct track *want = &rb->disc->tracks[i];
        const struct track *got = &copy->tracks[i];
        same = again.counts[i] == rb->counts[i] && got->start == want->start &&
               got->offset == want->offset && got->size == want->size &&
               got->data == want->data;
    }
    for (size_t i = FIRST; same && i < rb->count; i++) {
        const struct part *want = &rb->parts[i];
        const struct part *got = &again.parts[i];
        bool file = want->kind == GDI_FILE;
        same = got->kind == want->kind &&
               got->start == (file ? want->new_start : want->start) &&
               got->size == (file ? want->new_size : want->size) &&
               got->count == (file ? blocks(want->new_size) : want->count) &&
               got->record == want->record &&
               got->record_offset == want->record_offset &&
               (want->path == NULL
                    ? got->path == NULL
                    : got->path != NULL && strcmp(got->path, want->path) == 0);
    }
    int read = GDI_OK;
    for (size_t i = rb->root; same && i < rb->count; i++) {
        const struct part *file = &rb->parts[i];
        if (file->kind != GDI_FILE || !file->changed) {
            continue;
        }
        for (uint32_t k = 0; same && k < blocks(file->new_size); k++) {
            unsigned char got[BLOCK];
            unsigned char want[BLOCK];
            read = payload(rb, i, k, want);
            same = read == GDI_OK &&
                   sector(copy, file->new_start + k, got) == GDI_OK &&
                   memcmp(got, want, BLOCK) == 0;
        }
    }
    release(&again);
    gdi_close(copy);
    if (error == GDI_NOMEM) {
        return error;
    }
    if (read != GDI_OK) {
        return read;
    }
    if (!same) {
        errno = EIO;
        return fail(rb->disc, GDI_WRITE, staged);
    }
    return GDI_OK;
}

static int unchanged(struct rebuild *rb)
{
    // check size and ctime; this is not a content snapshot
    struct gdi *disc = rb->disc;
    for (size_t i = 0; i < disc->count; i++) {
        if (!stamp_matches(rb->files[i], &rb->stamps[i])) {
            return fail(disc, GDI_CHANGED, disc->tracks[i].path);
        }
    }
    if (!stamp_matches(disc->file, &rb->stamps[disc->count])) {
        return fail(disc, GDI_CHANGED, disc->image);
    }
    for (size_t i = rb->root; i < rb->count; i++) {
        const struct part *file = &rb->parts[i];
        if (file->from != NULL && !stamp_matches(file->from, &file->stamp)) {
            return fail(disc, GDI_CHANGED, file->source);
        }
    }
    return GDI_OK;
}

static int join(char out[PATH], const char *directory, const char *name,
                const char *suffix)
{
    // directory/name and then suffix, if that fits
    int length = snprintf(out, PATH, "%s/%s%s", directory, name, suffix);
    if (length < 0 || length >= PATH) {
        errno = ENAMETOOLONG;
        return GDI_WRITE;
    }
    return GDI_OK;
}

static int absent(const char *path)
{
    // GDI_OK if nothing has path's name, else GDI_WRITE, with errno EEXIST
    // if something does, or saying why looking for it failed. lstat sees a
    // dangling symlink as something.
    struct stat st;
    if (lstat(path, &st) == 0) {
        errno = EEXIST;
        return GDI_WRITE;
    }
    return errno == ENOENT ? GDI_OK : GDI_WRITE;
}

static int write_image(struct rebuild *rb, const char *directory)
{
    // write into an exclusively controlled directory. Stage the .gdi
    // until resurvey and input checks pass, then rename it into place.
    // Unlike hard links, rename works on FAT. Cleanup is best effort.
    struct gdi *disc = rb->disc;
    const char *name = base(disc->image);
    char path[PATH];
    char staged[PATH];
    char final[PATH];
    size_t made = 0;
    bool staging = false;
    int error = join(staged, directory, name, ".part");
    if (error == GDI_OK) {
        error = join(final, directory, name, "");
    }
    if (error != GDI_OK) {
        return fail(disc, error, directory);
    }
    rb->buffer = malloc(CHUNK);
    if (rb->buffer == NULL) {
        return GDI_NOMEM;
    }
    if (mkdir(directory, 0777) != 0) {
        return fail(disc, GDI_WRITE, directory);
    }
    while (error == GDI_OK && made < disc->count) {
        error = join(path, directory, base(disc->tracks[made].path), "");
        error = error == GDI_OK ? write_track(rb, made, path)
                                : fail(disc, error, directory);
        made += error == GDI_OK;
    }
    if (error == GDI_OK) {
        error = copy_gdi(rb, staged);
        staging = error == GDI_OK;
    }
    if (error == GDI_OK) {
        error = verify(rb, staged);
    }
    if (error == GDI_OK) {
        error = unchanged(rb);
    }
    if (error == GDI_OK && absent(final) != GDI_OK) {
        error = fail(disc, GDI_WRITE, final);
    }
    if (error == GDI_OK && rename(staged, final) != 0) {
        error = fail(disc, GDI_WRITE, final);
    }
    if (error != GDI_OK) {
        int saved = errno;
        for (size_t i = 0; i < made; i++) {
            if (join(path, directory, base(disc->tracks[i].path), "") ==
                GDI_OK) {
                (void)unlink(path);
            }
        }
        if (staging) {
            (void)unlink(staged);
        }
        (void)rmdir(directory);
        errno = saved;
    }
    return error;
}

static void emit(const struct rebuild *rb, size_t index, uint32_t start,
                 uint32_t count,
                 void (*each)(void *context, const struct gdi_region *region),
                 void *context)
{
    // a run of sectors in part index, as a region, with a changed file as
    // it was
    const struct part *part = &rb->parts[index];
    bool file = part->kind == GDI_FILE;
    int kind = index == rb->boot && file ? GDI_BOOT : part->kind;
    const char *path = part->path != NULL ? part->path : "";
    const struct gdi_region was = {
        .start = part->start,
        .count = part->count,
        .kind = kind,
        .path = path,
        .size = part->size,
    };
    const struct gdi_region region = {
        .start = start,
        .count = count,
        .kind = kind,
        .path = path,
        .size = file ? part->new_size : part->size,
        .was = file && part->changed ? &was : NULL,
    };
    each(context, &region);
}

static void report(const struct rebuild *rb,
                   void (*each)(void *context, const struct gdi_region *region),
                   void *context)
{
    // emit contiguous ownership runs by LBA, then zero-length regions
    // for files emptied by the plan
    for (uint32_t at = HIGH; at < rb->end;) {
        uint32_t index = rb->map[at - HIGH];
        uint32_t count = 1;
        while (at + count < rb->end && rb->map[at + count - HIGH] == index) {
            count++;
        }
        emit(rb, index, at, count, each, context);
        at += count;
    }
    for (size_t i = rb->root; i < rb->count; i++) {
        const struct part *file = &rb->parts[i];
        if (file->kind == GDI_FILE && file->changed && file->new_size == 0) {
            emit(rb, i, file->new_start, 0, each, context);
        }
    }
}

int gdi_rebuild(struct gdi *disc, const struct gdi_edit *edits, size_t count,
                const char *directory,
                void (*each)(void *context, const struct gdi_region *region),
                void *context)
{
    // a survey, the new image's names, a plan of the edits, checked whole,
    // then the image, or for a plan alone, a look at whether what the plan
    // read stayed as it was. An error no file took the blame for is about
    // the .gdi.
    struct rebuild rb = {.disc = disc};
    disc->failed[0] = '\0';
    int error = survey(&rb);
    if (error == GDI_OK) {
        error = check_output_names(disc);
    }
    if (error == GDI_OK) {
        error = apply_edits(&rb, edits, count);
    }
    if (error == GDI_OK) {
        error = validate_plan(&rb);
    }
    if (error == GDI_OK) {
        error =
            directory != NULL ? write_image(&rb, directory) : unchanged(&rb);
    }
    if (error == GDI_OK && each != NULL) {
        report(&rb, each, context);
    }
    release(&rb);
    if (error != GDI_OK && disc->failed[0] == '\0') {
        (void)fail(disc, error, disc->image);
    }
    return error;
}

void gdi_close(struct gdi *disc)
{
    if (disc == NULL) {
        return;
    }
    if (disc->file != NULL) {
        (void)fclose(disc->file);
    }
    for (size_t i = 0; i < disc->count; i++) {
        if (disc->tracks[i].file != NULL) {
            (void)fclose(disc->tracks[i].file);
        }
        free(disc->tracks[i].path);
    }
    free(disc);
}

const char *gdi_path(const struct gdi *disc)
{
    return disc->failed;
}

const char *gdi_strerror(int error)
{
    switch (error) {
    case GDI_OK:
        return "ok";
    case GDI_INVALID:
        return "not an image of a GD-ROM, or a damaged one";
    case GDI_MISSING:
        return "no such file or directory on the disc";
    case GDI_READ:
        return "can't read the file";
    case GDI_WRITE:
        return "can't write the file";
    case GDI_NOMEM:
        return "out of memory";
    case GDI_UNSUPPORTED:
        return "a disc, or a change to one, that gdi can't rebuild";
    case GDI_DUPLICATE:
        return "a file edited twice";
    case GDI_OVERLAP:
        return "a file placed over sectors it can't take";
    case GDI_NEEDS_PLACE:
        return "the file needs an explicit placement to grow";
    case GDI_CHANGED:
        return "the file changed after gdi read it";
    default:
        return "unknown error";
    }
}
