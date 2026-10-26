// gdi command-line interface

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gdi.h"

static const char usage[] =
    "usage: gdi list IMAGE [PATH...]\n"
    "       gdi extract IMAGE DIRECTORY [PATH...]\n"
    "       gdi map IMAGE [OPTION...]\n"
    "       gdi rebuild IMAGE DIRECTORY [OPTION...]\n"
    "\n"
    "options for map and rebuild:\n"
    "  --replace PATH FILE  use FILE's bytes for an existing disc file\n"
    "  --place PATH LBA     start an existing disc file at sector LBA\n";

static void print_file(void *context, const char *path, uint32_t size)
{
    (void)context;
    printf("%10" PRIu32 "  %s\n", size, path);
}

static const char *plural(uint32_t count)
{
    return count == 1 ? "" : "s";
}

static void print_region(void *context, const struct gdi_region *region)
{
    // map prints all ownership runs; rebuild prints only changed files
    static const char *const kinds[] = {
        [GDI_SYSTEM_AREA] = "system area",
        [GDI_DESCRIPTORS] = "volume descriptors",
        [GDI_PATH_TABLE] = "path table",
        [GDI_UNREFERENCED] = "unreferenced",
        [GDI_AUDIO] = "audio",
        [GDI_GAP] = "gap",
    };
    const bool *rebuilding = context;
    const struct gdi_region *was = region->was;
    if (*rebuilding && was == NULL) {
        return;
    }
    printf("%7" PRIu32 " %7" PRIu32, region->start, region->count);
    if (region->kind == GDI_DIRECTORY) {
        printf(" %10" PRIu32 "  %s/\n", region->size, region->path);
    } else if (region->kind != GDI_FILE && region->kind != GDI_BOOT) {
        printf(" %10s  %s\n", "", kinds[region->kind]);
    } else if (was == NULL) {
        printf(" %10" PRIu32 "  %s%s\n", region->size, region->path,
               region->kind == GDI_BOOT ? " (boot)" : "");
    } else {
        printf(" %10" PRIu32 "  %s (%swas %" PRIu32 " byte%s in %" PRIu32
               " sector%s at %" PRIu32 ")\n",
               region->size, region->path,
               region->kind == GDI_BOOT ? "boot; " : "", was->size,
               plural(was->size), was->count, plural(was->count), was->start);
    }
}

static int complain(const struct gdi *disc, const char *what, int error)
{
    // report the implicated path; exit 1 for invalid requests, 2 for
    // operational failures and usage errors, as GDI_DUPLICATE is here
    if (disc != NULL && *gdi_path(disc) != '\0') {
        what = gdi_path(disc);
    }
    if ((error == GDI_READ || error == GDI_WRITE) && errno != 0) {
        fprintf(stderr, "gdi: %s: %s: %s\n", what, gdi_strerror(error),
                strerror(errno));
    } else {
        fprintf(stderr, "gdi: %s: %s\n", what, gdi_strerror(error));
    }
    switch (error) {
    case GDI_INVALID:
    case GDI_MISSING:
    case GDI_UNSUPPORTED:
    case GDI_OVERLAP:
    case GDI_NEEDS_PLACE:
        return 1;
    default:
        return 2;
    }
}

static bool decimal(const char *text, uint32_t *value)
{
    // require a nonempty unsigned decimal LBA within uint32_t
    uint64_t sum = 0;
    for (const char *at = text; *at != '\0'; at++) {
        if (*at < '0' || *at > '9') {
            return false;
        }
        sum = (sum * 10) + (uint64_t)(*at - '0');
        if (sum > UINT32_MAX) {
            return false;
        }
    }
    *value = (uint32_t)sum;
    return *text != '\0';
}

static bool paths_match(const char *one, const char *other)
{
    // ignore leading, trailing and repeated slashes when merging edits
    for (;;) {
        one += strspn(one, "/");
        other += strspn(other, "/");
        size_t length = strcspn(one, "/");
        if (strcspn(other, "/") != length || strncmp(one, other, length) != 0) {
            return false;
        }
        if (length == 0) {
            return true;
        }
        one += length;
        other += length;
    }
}

static bool parse_edits(int argc, char **argv, int at, struct gdi_edit *edits,
                        size_t *count)
{
    // merge replace/place options by canonical path. A repeated field
    // starts a second edit so the library reports GDI_DUPLICATE. edits
    // needs (argc - at) / 3 slots.
    *count = 0;
    while (at < argc) {
        bool replace = strcmp(argv[at], "--replace") == 0;
        if ((!replace && strcmp(argv[at], "--place") != 0) || argc - at < 3) {
            return false;
        }
        struct gdi_edit *edit = NULL;
        for (size_t i = 0; i < *count && edit == NULL; i++) {
            if (paths_match(edits[i].path, argv[at + 1])) {
                edit = &edits[i];
            }
        }
        if (edit == NULL || (replace ? edit->from != NULL : edit->start != 0)) {
            edit = &edits[(*count)++];
            *edit = (struct gdi_edit){argv[at + 1], NULL, 0};
        }
        if (replace) {
            edit->from = argv[at + 2];
        } else if (!decimal(argv[at + 2], &edit->start) || edit->start == 0) {
            return false;
        }
        at += 3;
    }
    return true;
}

static int rebuild(struct gdi *disc, const char *image,
                   const struct gdi_edit *edits, size_t count,
                   const char *directory)
{
    bool rebuilding = directory != NULL;
    errno = 0;
    int error =
        gdi_rebuild(disc, edits, count, directory, print_region, &rebuilding);
    return error != GDI_OK ? complain(disc, image, error) : 0;
}

static int paths(struct gdi *disc, const char *image, char **names, int count,
                 const char *directory)
{
    // missing disc paths are recoverable; I/O errors stop the command
    int status = 0;
    for (int i = 0; i < count || i == 0; i++) {
        const char *path = i < count ? names[i] : "";
        errno = 0;
        int error = directory == NULL ? gdi_list(disc, path, print_file, NULL)
                                      : gdi_extract(disc, path, directory);
        if (error != GDI_OK) {
            int code = complain(disc, image, error);
            status = code > status ? code : status;
            if (code == 2) {
                break;
            }
        }
    }
    return status;
}

int main(int argc, char **argv)
{
    // reject malformed options before opening the image
    const char *command = argc >= 2 ? argv[1] : "";
    bool list = argc >= 3 && strcmp(command, "list") == 0;
    bool extract = argc >= 4 && strcmp(command, "extract") == 0;
    bool map = argc >= 3 && strcmp(command, "map") == 0;
    bool build = argc >= 4 && strcmp(command, "rebuild") == 0;
    struct gdi_edit *edits = calloc((size_t)argc, sizeof *edits);
    size_t count = 0;
    int status = 2;
    if (edits == NULL) {
        fputs("gdi: out of memory\n", stderr);
    } else if ((!list && !extract && !map && !build) ||
               ((map || build) &&
                !parse_edits(argc, argv, map ? 3 : 4, edits, &count))) {
        fputs(usage, stderr);
    } else {
        struct gdi *disc = NULL;
        errno = 0;
        int error = gdi_open(&disc, argv[2]);
        if (error != GDI_OK) {
            status = complain(disc, argv[2], error);
        } else if (map || build) {
            status =
                rebuild(disc, argv[2], edits, count, build ? argv[3] : NULL);
        } else {
            int first = list ? 3 : 4;
            status = paths(disc, argv[2], argv + first, argc - first,
                           extract ? argv[3] : NULL);
        }
        gdi_close(disc);
        errno = 0;
        if (fflush(stdout) != 0 || ferror(stdout) != 0) {
            status = complain(NULL, "stdout", GDI_WRITE);
        }
    }
    free(edits);
    return status;
}
