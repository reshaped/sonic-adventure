/* the PC backend's storage: a save is the file its name gives. a request
 * ends in its call, which posts its SA_EVENT_STORAGE before it returns. so
 * the call's result is the request's. a read of a save larger than its
 * buffer gives SA_ERR_FULL, and of a missing file SA_ERR_MISSING. a write
 * makes a new file beside the save, and any missing directory above. once
 * its bytes are on the disk, the new file is renamed over the save. so a
 * write cut short leaves the save as it was */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
/* minwindef.h's empty near and far would erase sa_engine.h's fields */
#undef near
#undef far
#define FLAGS (O_BINARY | O_NOINHERIT)
/* either slash ends a directory's name */
#define SLASHES "/\\"
#define make_directory(path) _mkdir(path)
#define fsync _commit
/* rename fails where the save exists, and MoveFileEx replaces it */
#define take_place(fresh, name)                                                \
    (MoveFileExA(fresh, name,                                                  \
                 MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)           \
         ? 0                                                                   \
         : -1)
#else
#define FLAGS O_CLOEXEC
#define SLASHES "/"
#define make_directory(path) mkdir(path, 0755)
#define take_place(fresh, name) rename(fresh, name)
#endif

#include "runtime.h"

namespace {

/* take a request's handle where its event has room */
sa_result begin(sa_engine &engine, sa_handle &handle)
{
    if (engine.event_count + engine.event_reserved >= engine.desc.events) {
        return SA_ERR_FULL;
    }
    handle = engine.resources.take(SA_KIND_REQUEST);
    return handle != SA_NULL ? SA_OK : SA_ERR_FULL;
}

/* post the request's event in the room begin found. give its handle back */
sa_result end(sa_engine &engine, sa_handle handle, uint32_t bytes,
              sa_handle *request)
{
    sa_event event{};

    sa::event_reserve(engine);
    event.kind = SA_EVENT_STORAGE;
    event.handle = handle;
    event.result = static_cast<int32_t>(bytes);
    event.tick = engine.tick;
    event.time_ns = sa_clock_ns(&engine);
    sa::event_post(engine, event);
    engine.resources.give(handle);
    *request = handle;
    return SA_OK;
}

/* path's file, made to write, and each directory above it where none is */
int made(std::string path)
{
    const int flags = O_WRONLY | O_CREAT | O_TRUNC | FLAGS;
    int fd = open(path.c_str(), flags, 0644);

    if (fd < 0 && errno == ENOENT) {
        for (size_t at = path.find_first_of(SLASHES, 1);
             at != std::string::npos;
             at = path.find_first_of(SLASHES, at + 1)) {
            char slash = path[at];

            path[at] = '\0';
            make_directory(path.c_str());
            path[at] = slash;
        }
        fd = open(path.c_str(), flags, 0644);
    }
    return fd;
}

bool written(int fd, const uint8_t *data, uint32_t bytes)
{
    while (bytes != 0) {
        ssize_t wrote = write(fd, data, bytes);

        if (wrote < 0 && errno == EINTR) {
            continue;
        }
        if (wrote <= 0) {
            return false;
        }
        data += wrote;
        bytes -= static_cast<uint32_t>(wrote);
    }
    return true;
}

} // namespace

sa_result sa_storage_read(sa_engine *engine, const char *name, void *buffer,
                          uint32_t bytes, sa_handle *request)
{
    sa_handle handle = SA_NULL;
    struct stat about;
    FILE *in;
    sa_result result;
    bool whole;

    if (engine == nullptr || name == nullptr || request == nullptr ||
        (buffer == nullptr && bytes != 0)) {
        return SA_ERR_INVALID;
    }
    if (!(in = fopen(name, "rb"))) {
        return errno == ENOENT ? SA_ERR_MISSING : SA_ERR_IO;
    }
    if (fstat(fileno(in), &about) != 0) {
        fclose(in);
        return SA_ERR_IO;
    }
    if (static_cast<uint64_t>(about.st_size) > bytes) {
        fclose(in);
        return SA_ERR_FULL;
    }
    if ((result = begin(*engine, handle)) != SA_OK) {
        fclose(in);
        return result;
    }
    whole = fread(buffer, 1, static_cast<size_t>(about.st_size), in) ==
            static_cast<size_t>(about.st_size);
    fclose(in);
    if (!whole) {
        engine->resources.give(handle);
        return SA_ERR_IO;
    }
    return end(*engine, handle, static_cast<uint32_t>(about.st_size), request);
}

sa_result sa_storage_write(sa_engine *engine, const char *name,
                           const void *data, uint32_t bytes, sa_handle *request)
{
    sa_handle handle = SA_NULL;
    std::string fresh;
    sa_result result;
    int fd;
    bool done;

    if (engine == nullptr || name == nullptr || request == nullptr ||
        (data == nullptr && bytes != 0)) {
        return SA_ERR_INVALID;
    }
    if ((result = begin(*engine, handle)) != SA_OK) {
        return result;
    }
    fresh = std::string(name) + ".new";
    fd = made(fresh);
    done = fd >= 0 && written(fd, static_cast<const uint8_t *>(data), bytes) &&
           fsync(fd) == 0;
    if (fd >= 0) {
        done = close(fd) == 0 && done;
    }
    if (!done || take_place(fresh.c_str(), name) != 0) {
        unlink(fresh.c_str());
        engine->resources.give(handle);
        return SA_ERR_IO;
    }
    return end(*engine, handle, bytes, request);
}
