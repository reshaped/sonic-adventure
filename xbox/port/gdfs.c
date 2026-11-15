/* Shinobi's GD-ROM file system, gdFs, on the host's files: the port layer's
 * side of sdk/shinobi/gdfs*.c, which make pc leaves out.
 *
 * the current directory is a path from the disc's root, and each handle a
 * host file with its next sector, counted from the handle's first: a file's
 * own, or a range's, which gdFsOpenRange opens.
 *
 * the host's files have no place on a disc, so the port gives each file it
 * names a frame address of its own, gdFsGetDirInfo's fad, in a table: one
 * file after another, from FIRST_FAD. a range of frame addresses, as the ADX
 * library opens a file by its fad and a partition's files by their offsets
 * in it, opens the file whose addresses hold it. a read reads whole sectors, as the drive
 * does, up to the file's end. a requested read, gdFsReqRd32, completes as
 * it is made: the handle reads complete, and its read-end callback runs at
 * the next gdFsExecServer, as the drive's interrupt would have run it */
#include <shinobi.h>
#include <sg_xpt.h>

#include "xbox/port/host.h"
#include "xbox/port/port.h"

#define HANDLES 32
#define SECTOR GDD_FS_SCTSIZE
#define FILES 512
#define FIRST_FAD 45150 /* the high-density area's first, as on the disc */

struct GDS_FS_HANDLE {
    void *file; /* the host's, or null for a closed handle */
    Sint32 size; /* in bytes, from first */
    Sint32 first; /* the handle's first sector in the host's file */
    Sint32 next; /* the next sector to read, from first */
    Sint32 stat;
    Sint32 error;
    Sint32 last_read; /* the sectors the last request read */
    GDFS_FUNC read_end;
    void *read_end_obj;
    Bool read_end_due;
};

static struct GDS_FS_HANDLE handles[HANDLES];
static char current[128] = "";

/* each file the port has given a frame address: its path from the root, its
 * first frame address, and its sectors */
static struct {
    char path[96];
    Sint32 fad;
    Sint32 sectors;
} files[FILES];
static int file_count;
static Sint32 next_fad = FIRST_FAD;
static GDFS_ERRFUNC error_func;
static void *error_obj;
static Sint32 global_error;

/* path joined to the current directory, or from the root where it starts
 * with a slash or a backslash, into out */
static void join(const char *name, char *out, int room)
{
    int k = 0;
    const char *from;

    if (name[0] == '/' || name[0] == '\\') {
        from = name + 1;
    } else {
        for (from = current; *from != '\0' && k < room - 2; from++) {
            out[k++] = *from;
        }
        if (k > 0) {
            out[k++] = '/';
        }
        from = name;
    }
    for (; *from != '\0' && k < room - 1; from++) {
        out[k++] = *from == '\\' ? '/' : *from;
    }
    out[k] = '\0';
}

static int same(const char *a, const char *b)
{
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

/* path's frame address, given it on first sight, for a file of size bytes.
 * -1 where the table is full */
static Sint32 file_fad(const char *path, Sint32 size)
{
    int k;

    for (k = 0; k < file_count; k++) {
        if (same(files[k].path, path)) {
            return files[k].fad;
        }
    }
    if (file_count == FILES) {
        port_log("gdfs: the table of frame addresses is full");
        return -1;
    }
    for (k = 0; path[k] != '\0' && k < (int)sizeof files[0].path - 1; k++) {
        files[file_count].path[k] = path[k];
    }
    files[file_count].path[k] = '\0';
    files[file_count].fad = next_fad;
    files[file_count].sectors = (size + SECTOR - 1) / SECTOR;
    /* a sector apart, so a range's end never reads as the next file's start */
    next_fad += files[file_count].sectors + 1;
    return files[file_count++].fad;
}

/* a free handle over the host's file, from sector first, size bytes long, or
 * null */
static GDFS open_handle(void *file, Sint32 first, Sint32 size)
{
    int k;

    for (k = 0; k < HANDLES && handles[k].file != NULL; k++) {
    }
    if (k == HANDLES) {
        HOST->file_close(file);
        global_error = GDD_ERR_NOHNDL;
        return NULL;
    }
    handles[k].file = file;
    handles[k].size = size;
    handles[k].first = first;
    handles[k].next = 0;
    handles[k].stat = GDD_STAT_IDLE;
    handles[k].error = GDD_ERR_OK;
    handles[k].last_read = 0;
    handles[k].read_end = NULL;
    handles[k].read_end_due = FALSE;
    global_error = GDD_ERR_OK;
    return &handles[k];
}

Sint32 gdFsInit(Sint32 max_open, void *gdfs_work, Sint32 max_dirent, void *dirbuf)
{
    (void)max_open;
    (void)gdfs_work;
    (void)max_dirent;
    (void)dirbuf;
    current[0] = '\0';
    return GDD_ERR_OK;
}

Sint32 gdFsInitOvl(Sint32 max_open, void *gdfs_work, Sint32 max_dirent, void *dirbuf)
{
    return gdFsInit(max_open, gdfs_work, max_dirent, dirbuf);
}

void gdFsFinish(void)
{
    int k;

    for (k = 0; k < HANDLES; k++) {
        gdFsClose(&handles[k]);
    }
}

/* change to dirname: "..", a name in the current directory, or a path from
 * the root */
Sint32 gdFsChangeDir(const char *dirname)
{
    char path[128];
    int k;

    if (dirname[0] == '.' && dirname[1] == '.' && dirname[2] == '\0') {
        for (k = 0; current[k] != '\0'; k++) {
        }
        while (k > 0 && current[k - 1] != '/') {
            k--;
        }
        current[k > 0 ? k - 1 : 0] = '\0';
        return GDD_ERR_OK;
    }
    join(dirname, path, sizeof path);
    if (path[0] != '\0' && !HOST->dir_exists(path)) {
        return GDD_ERR_NOTFOUND;
    }
    for (k = 0; path[k] != '\0' && k < (int)sizeof current - 1; k++) {
        current[k] = path[k];
    }
    current[k] = '\0';
    return GDD_ERR_OK;
}

GDFS gdFsOpen(const char *fname, GDFS_DIRREC gf_dirrec)
{
    char path[160];
    void *file;

    (void)gf_dirrec;
    join(fname, path, sizeof path);
    file = HOST->file_open(path);
    if (file == NULL) {
        global_error = GDD_ERR_NOTFOUND;
        return NULL;
    }
    return open_handle(file, 0, HOST->file_size(file));
}

/* nsct sectors from frame address stsct, within the file the port gave
 * those addresses (file_fad) */
GDFS gdFsOpenRange(Sint32 stsct, Sint32 nsct)
{
    Sint32 first;
    Sint32 size;
    void *file;
    int k;

    for (k = 0; k < file_count; k++) {
        if (stsct >= files[k].fad && stsct < files[k].fad + files[k].sectors) {
            break;
        }
    }
    if (k == file_count || nsct < 0) {
        global_error = GDD_ERR_NOTFOUND;
        return NULL;
    }
    file = HOST->file_open(files[k].path);
    if (file == NULL) {
        global_error = GDD_ERR_NOTFOUND;
        return NULL;
    }
    first = stsct - files[k].fad;
    size = HOST->file_size(file) - first * SECTOR;
    if (size > nsct * SECTOR) {
        size = nsct * SECTOR;
    }
    return open_handle(file, first, size);
}

void gdFsClose(GDFS gdfs)
{
    if (gdfs != NULL && gdfs->file != NULL) {
        HOST->file_close(gdfs->file);
        gdfs->file = NULL;
    }
}

/* the host's file behind gdfs, which the handle gives up and closes: the
 * port's movie player reads it by offset, from the file's start */
void *port_gdfs_take_file(GDFS gdfs)
{
    void *file = gdfs->file;

    gdfs->file = NULL;
    return file;
}

Sint32 gdFsGetDirInfo(const char *name, GDFS_DIRINFO *dirinfo)
{
    char path[160];
    void *file;

    join(name, path, sizeof path);
    file = HOST->file_open(path);
    if (file == NULL) {
        if (HOST->dir_exists(path)) {
            dirinfo->fad = 0;
            dirinfo->fsize = 0;
            dirinfo->flag = GDD_FF_DIRECTORY;
            return GDD_ERR_OK;
        }
        return GDD_ERR_NOTFOUND;
    }
    dirinfo->fsize = HOST->file_size(file);
    dirinfo->fad = file_fad(path, dirinfo->fsize);
    dirinfo->flag = 0;
    HOST->file_close(file);
    return GDD_ERR_OK;
}

Bool gdFsGetFileSize(GDFS gdfs, Sint32 *fsize)
{
    if (gdfs == NULL || gdfs->file == NULL) {
        return FALSE;
    }
    *fsize = gdfs->size;
    return TRUE;
}

Bool gdFsGetFileSctSize(GDFS gdfs, Sint32 *fsctsize)
{
    if (gdfs == NULL || gdfs->file == NULL) {
        return FALSE;
    }
    *fsctsize = (gdfs->size + SECTOR - 1) / SECTOR;
    return TRUE;
}

Sint32 gdFsSeek(GDFS gdfs, Sint32 sctno, Sint32 type)
{
    Sint32 sectors = (gdfs->size + SECTOR - 1) / SECTOR;
    Sint32 at = type == GDD_SEEK_SET ? sctno : type == GDD_SEEK_CUR ? gdfs->next + sctno : sectors + sctno;

    if (at < 0 || at > sectors) {
        return GDD_ERR_SEEK;
    }
    gdfs->next = at;
    return GDD_ERR_OK;
}

Sint32 gdFsTell(GDFS gdfs)
{
    return gdfs->next;
}

Sint32 gdFsCheckEof(GDFS gdfs, Bool *iseof)
{
    *iseof = gdfs->next * SECTOR >= gdfs->size;
    return GDD_ERR_OK;
}

/* read nsct sectors from the next, as far as the file goes */
static Sint32 read_sectors(GDFS gdfs, Sint32 nsct, void *buf)
{
    Sint32 offset = gdfs->next * SECTOR;
    Sint32 at = (gdfs->first + gdfs->next) * SECTOR;
    Sint32 count = nsct * SECTOR;
    Sint32 got;

    if (gdfs == NULL || gdfs->file == NULL) {
        return GDD_ERR_ILLHNDL;
    }
    if (offset + count > gdfs->size) {
        count = gdfs->size > offset ? gdfs->size - offset : 0;
    }
    got = count > 0 ? HOST->file_read(gdfs->file, (unsigned)at, buf, count) : 0;
    gdfs->last_read = (got + SECTOR - 1) / SECTOR;
    gdfs->next += gdfs->last_read;
    gdfs->stat = GDD_STAT_COMPLETE;
    gdfs->error = GDD_ERR_OK;
    return GDD_ERR_OK;
}

Sint32 gdFsRead(GDFS gdfs, Sint32 nsct, void *buf)
{
    return read_sectors(gdfs, nsct, buf);
}

/* a requested read returns the sectors it takes, as Shinobi's does, 0 at the
 * end, or an error. the ADX library's stream counts on the sectors */
Sint32 gdFsReqRd32(GDFS gdfs, Sint32 nsct, void *buf)
{
    Sint32 result = read_sectors(gdfs, nsct, buf);

    if (result != GDD_ERR_OK) {
        return result;
    }
    if (gdfs->read_end != NULL) {
        gdfs->read_end_due = TRUE;
    }
    return gdfs->last_read;
}

Sint32 gdFsGetStat(GDFS gdfs)
{
    return gdfs != NULL ? gdfs->stat : GDD_STAT_IDLE;
}

Sint32 gdFsGetErrStat(GDFS gdfs)
{
    return gdfs != NULL ? gdfs->error : global_error;
}

Sint32 gdFsStopRd(GDFS gdfs)
{
    if (gdfs != NULL) {
        gdfs->stat = GDD_STAT_IDLE;
    }
    return GDD_ERR_OK;
}

Sint32 gdFsGetNumRd(GDFS gdfs)
{
    return gdfs->last_read;
}

Sint32 gdFsGetDrvStat(void)
{
    return 0x02; /* paused, with a disc in the drive */
}

Sint32 gdFsReqDrvStat(void)
{
    return GDD_ERR_OK;
}

Bool gdFsIsTrayOpen(void)
{
    return FALSE;
}

void gdFsEntryRdEndFunc(GDFS gdfs, GDFS_FUNC func, void *obj)
{
    gdfs->read_end = func;
    gdfs->read_end_obj = obj;
}

void gdFsEntryErrFunc(GDFS gdfs, GDFS_ERRFUNC erfunc, void *obj)
{
    (void)gdfs;
    (void)erfunc;
    (void)obj;
}

void gdFsEntryErrFuncAll(GDFS_ERRFUNC erfunc, void *obj)
{
    error_func = erfunc;
    error_obj = obj;
}

/* run each read-end callback that a completed request owes */
void gdFsExecServer(void)
{
    int k;

    for (k = 0; k < HANDLES; k++) {
        if (handles[k].read_end_due) {
            handles[k].read_end_due = FALSE;
            handles[k].read_end(handles[k].read_end_obj);
        }
    }
}
