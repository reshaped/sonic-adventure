// Dreamcast GDI image reader and rebuilder
#ifndef GDI_H
#define GDI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    GDI_OK,
    GDI_INVALID,     // malformed or damaged image
    GDI_MISSING,     // disc path not found
    GDI_READ,        // input I/O failure; errno may provide details
    GDI_WRITE,       // output I/O failure; errno may provide details
    GDI_NOMEM,       // allocation failed
    GDI_UNSUPPORTED, // image or edit the rebuilder cannot handle
    GDI_DUPLICATE,   // more than one edit for a disc file
    GDI_OVERLAP,     // placement claims an occupied or unusable sector
    GDI_NEEDS_PLACE, // growth into more sectors requires an explicit placement
    GDI_CHANGED,     // input changed during the operation
};

struct gdi;

// Pointer arguments must be valid unless a comment says they may be NULL.
// Listing, extraction and rebuilding need a handle that gdi_open opened
// successfully. context is passed to the callback unread and may be NULL.

// Open image and its data tracks. On every return, close a non-NULL *disc.
// After a failed open it serves only gdi_path and gdi_close; gdi_path may be
// "" when no file is to blame, as after GDI_NOMEM. Each handle is
// single-threaded; handles share no mutable state.
int gdi_open(struct gdi **disc, const char *image);

// Report files at or below path, or the whole disc for "". Names omit
// ISO9660 versions and use /. A null callback still validates the traversal.
// Each path is valid only during its callback; do not reenter or close disc.
int gdi_list(struct gdi *disc, const char *path,
             void (*each)(void *context, const char *path, uint32_t size),
             void *context);

// Extract path ("" for the whole disc) beneath directory ("" for cwd),
// creating missing directories. Existing files are never replaced
// (errno EEXIST). Partial files may remain after failure. Existing
// destination-directory symlinks are followed.
int gdi_extract(struct gdi *disc, const char *path, const char *directory);

// One disc file's replacement bytes, placement, or both. A null from keeps
// its original bytes; start == 0 keeps its original LBA.
struct gdi_edit {
    const char *path; // disc path, as returned by gdi_list
    const char *from; // replacement host file, or NULL
    uint32_t start;   // absolute first sector (LBA), or 0
};

// Sector ownership reported by gdi_rebuild.
enum {
    GDI_SYSTEM_AREA,  // bootstrap sectors before the descriptors
    GDI_DESCRIPTORS,  // volume descriptors through the terminator
    GDI_PATH_TABLE,   // path-table copy
    GDI_DIRECTORY,    // directory records
    GDI_FILE,         // file, including final-sector padding
    GDI_BOOT,         // file named by the Katana boot header
    GDI_UNREFERENCED, // not claimed by an ISO9660 structure or file
    GDI_AUDIO,        // audio track
    GDI_GAP,          // outside track extents
};

// A contiguous ownership run. start is its first absolute LBA, and count its
// length in 2048-byte logical sectors, not raw track bytes. path is the disc
// path, "" for the root and unnamed regions; size is the whole file's or
// directory's bytes, 0 for unnamed regions. was is the file as it was when
// its bytes, start or size changed, a payload-only change included, and
// NULL otherwise.
struct gdi_region {
    uint32_t start;
    uint32_t count;
    int kind;
    const char *path;
    uint32_t size;
    const struct gdi_region *was;
};

// Plan edits without writing (directory == NULL), or rebuild into a new
// directory that remains under the caller's exclusive control. No file may
// occur twice in edits, which may be NULL when count is 0. Input image and
// replacement files must remain unchanged throughout the call. The source
// image is never written. A NULL callback reports nothing.
//
// The callback receives sector runs in LBA order, from 45000 to the volume's
// end, followed by zero-length regions for emptied files. No callbacks occur
// on error. Region pointers, paths, and was are valid only during the
// callback; do not reenter or close disc. The handle remains usable after
// failure; output cleanup is best effort.
// See REBUILD.md for placement rules and supported images.
int gdi_rebuild(struct gdi *disc, const struct gdi_edit *edits, size_t count,
                const char *directory,
                void (*each)(void *context, const struct gdi_region *region),
                void *context);

// Path associated with the last error (host or disc), or "" after success.
// Owned by disc; valid until its next operation or close.
const char *gdi_path(const struct gdi *disc);

// Accepts NULL.
void gdi_close(struct gdi *disc);

// Static text; never NULL, even for an unknown error.
const char *gdi_strerror(int error);

#ifdef __cplusplus
}
#endif

#endif
