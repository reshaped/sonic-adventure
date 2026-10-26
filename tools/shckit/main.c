// shckit command-line interface

// stat requires POSIX
// NOLINTBEGIN(bugprone-reserved-identifier,readability-identifier-naming)
#define _POSIX_C_SOURCE 200809L
// NOLINTEND(bugprone-reserved-identifier,readability-identifier-naming)

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "shckit.h"

static const char usage[] =
    "usage: shckit apply -p PATCH ... -o OUT IN\n"
    "       shckit check FILE...\n"
    "       shckit lint FILE...\n"
    "       shckit inspect --release RELEASE --mode MODE --entry LABEL\n"
    "                      [--end LABEL] [--sz 0|1] [--all] FILE\n"
    "       shckit translate -o OUT IN\n";

// Exit statuses. BAD is a text shckit refuses; FAILED is a command-line,
// I/O or memory error, and outranks BAD. UNFINISHED is an inspection that
// met a limit.
enum { OK, BAD, FAILED, UNFINISHED };

static void complain(const char *path)
{
    fprintf(stderr, "shckit: %s: %s\n", path,
            errno != 0 ? strerror(errno) : "input/output error");
}

static int read_file(const char *path, unsigned char **data, size_t *len)
{
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
    // a full buffer may not hold the whole file
    while (ok && size == cap) {
        cap = cap != 0 ? cap * 2 : (size_t)1 << 16;
        grown = cap > size ? realloc(buf, cap) : NULL; // unless cap overflowed
        if (grown == NULL) {
            // C99 does not require realloc to set errno
            fprintf(stderr, "shckit: %s: out of memory\n", path);
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

static int write_file(const char *path, const void *data, size_t len)
{
    // fclose can report buffered write errors. Leave a partial file in
    // place: the destination may be a device or a symlink.
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

static bool same_file(const char *one, const char *two)
{
    // whether two paths, however spelled and through any links, name one
    // file. A path to nothing names none.
    struct stat first;
    struct stat second;
    return stat(one, &first) == 0 && stat(two, &second) == 0 &&
           first.st_dev == second.st_dev && first.st_ino == second.st_ino;
}

static int refused(const char *path, int error, size_t line, int patch)
{
    // report a refusal with its original line and patch when it has them.
    // Running out of memory is not the text's fault.
    if (error == SHCKIT_NOMEM) {
        fprintf(stderr, "shckit: %s\n", shckit_strerror(error));
        return FAILED;
    }
    fprintf(stderr, "shckit: %s", path);
    if (line != 0) {
        fprintf(stderr, ":%zu", line);
    }
    if (patch >= 0) {
        fprintf(stderr, ": %s", shckit_patch_name(patch));
    }
    fprintf(stderr, ": %s\n", shckit_strerror(error));
    return BAD;
}

static int load(const char *path, struct shckit_text **text)
{
    unsigned char *bytes = NULL;
    size_t len = 0;
    size_t line = 0;
    int error = SHCKIT_OK;

    if (read_file(path, &bytes, &len) != 0) {
        return FAILED;
    }
    error = shckit_read(text, bytes, len, &line);
    free(bytes);
    if (error != SHCKIT_OK) {
        return refused(path, error, line, -1);
    }
    return OK;
}

struct request {
    unsigned patches;
    const char *in;
    const char *out;
};

static bool read_request(int argc, char **argv, struct request *request,
                         bool patching)
{
    // parse -p PATCH at least once when patching and never otherwise, -o OUT
    // once, and IN last, and refuse whatever the arguments alone decide
    // before IN is read. On failure, print the problem and return false.
    const unsigned pools =
        1U << SHCKIT_ALIGN_POOL | 1U << SHCKIT_USE_ASSEMBLER_POOLS;
    for (int i = 2; i < argc; i++) {
        const char *option = argv[i];
        if (i == argc - 1 && option[0] != '-') {
            request->in = option;
            break;
        }
        if (i == argc - 1 || strlen(option) != 2 || option[0] != '-') {
            break;
        }
        if (option[1] == 'p' && patching) {
            int patch = shckit_patch(argv[++i]);
            if (patch < 0) {
                fprintf(stderr, "shckit: no patch is called %s\n", argv[i]);
                return false;
            }
            if ((request->patches & 1U << patch) != 0) {
                fprintf(stderr, "shckit: %s is named twice\n", argv[i]);
                return false;
            }
            request->patches |= 1U << patch;
        } else if (option[1] == 'o' && request->out == NULL) {
            request->out = argv[++i];
        } else {
            break;
        }
    }
    if (request->in == NULL || request->out == NULL ||
        (patching && request->patches == 0)) {
        fputs(usage, stderr);
        return false;
    }
    // shckit_apply refuses this pair too, but only after IN has been read
    if ((request->patches & pools) == pools) {
        fprintf(stderr, "shckit: %s\n", shckit_strerror(SHCKIT_CONFLICT));
        return false;
    }
    return true;
}

static int rewrite_file(int argc, char **argv, bool patching)
{
    // apply when patching, translate otherwise: write OUT from IN, or nothing
    // when IN is refused
    struct request request = {0, NULL, NULL};
    struct shckit_text *text = NULL;
    unsigned char *bytes = NULL;
    size_t len = 0;
    size_t line = 0;
    int patch = -1;
    int error = SHCKIT_OK;
    int status = OK;

    if (!read_request(argc, argv, &request, patching)) {
        return FAILED;
    }
    if (same_file(request.in, request.out)) {
        fprintf(stderr, "shckit: %s is the input\n", request.out);
        return FAILED;
    }
    status = load(request.in, &text);
    if (status != OK) {
        return status;
    }
    error = patching ? shckit_apply(text, request.patches, &line, &patch)
                     : shckit_translate(text, NULL, 0, &len, &line);
    if (error != SHCKIT_OK && error != SHCKIT_FULL) {
        status = refused(request.in, error, line, patch);
    } else {
        // a size past SIZE_MAX is a size no buffer holds
        if (patching) {
            error = shckit_write(text, NULL, 0, &len);
        }
        bytes = error == SHCKIT_OK ? malloc(len != 0 ? len : 1) : NULL;
        if (bytes == NULL ||
            (patching ? shckit_write(text, bytes, len, NULL)
                      : shckit_translate(text, bytes, len, NULL, NULL)) !=
                SHCKIT_OK) {
            status = refused(request.in, SHCKIT_NOMEM, 0, -1);
        } else {
            status = write_file(request.out, bytes, len) == 0 ? OK : FAILED;
        }
    }
    free(bytes);
    shckit_free(text);
    return status;
}

typedef int ruleset(const struct shckit_text *text,
                    struct shckit_finding **found, size_t *count);

static int report(const char *path, const struct shckit_text *text,
                  ruleset *rules)
{
    // name the original line number, not the index
    struct shckit_finding *found = NULL;
    size_t count = 0;
    int status = OK;

    if (rules(text, &found, &count) != SHCKIT_OK) {
        status = refused(path, SHCKIT_NOMEM, 0, -1);
    }
    for (size_t i = 0; i < count; i++) {
        fprintf(stderr, "shckit: %s:%zu: %s: %s\n", path,
                shckit_line(text, found[i].index)->input,
                shckit_rule_name(found[i].rule),
                shckit_rule_definition(found[i].rule));
        status = BAD;
    }
    free(found);
    return status;
}

static int check_file(const char *path, ruleset *rules)
{
    struct shckit_text *text = NULL;
    int status = load(path, &text);

    if (status == OK) {
        status = report(path, text, rules);
    }
    shckit_free(text);
    return status;
}

static int check_files(int argc, char **argv, ruleset *rules)
{
    // check every file, so a refused one hides none after it, and return the
    // worst status
    int status = OK;
    for (int i = 2; i < argc; i++) {
        int one = check_file(argv[i], rules);
        status = one > status ? one : status;
    }
    return status;
}

struct inspect_request {
    struct shckit_producer producer;
    const char *entry;
    const char *end;
    const char *in;
    int sz;
    bool all;
};

static bool read_inspect_request(int argc, char **argv,
                                 struct inspect_request *request)
{
    // parse each option once, in any order, and FILE last
    static const struct {
        const char *name;
        unsigned modes;
    } modes[] = {
        {"machinecode", SHCKIT_MACHINECODE},
        {"asmcode", SHCKIT_ASMCODE},
        {"both", SHCKIT_MACHINECODE | SHCKIT_ASMCODE},
    };
    const char *mode = NULL;
    const char *sz = NULL;
    bool ok = true;
    struct {
        const char *name;
        const char **value;
    } options[] = {
        {"--release", &request->producer.release},
        {"--mode", &mode},
        {"--entry", &request->entry},
        {"--end", &request->end},
        {"--sz", &sz},
    };
    const size_t count = sizeof options / sizeof options[0];

    for (int i = 2; i < argc; i++) {
        size_t option = 0;
        if (i == argc - 1 && argv[i][0] != '-') {
            request->in = argv[i];
            break;
        }
        if (strcmp(argv[i], "--all") == 0 && !request->all) {
            request->all = true;
            continue;
        }
        while (option < count && strcmp(argv[i], options[option].name) != 0) {
            option++;
        }
        if (i == argc - 1 || option == count ||
            *options[option].value != NULL) {
            break;
        }
        *options[option].value = argv[++i];
    }
    for (size_t i = 0; mode != NULL && i < sizeof modes / sizeof modes[0];
         i++) {
        if (strcmp(mode, modes[i].name) == 0) {
            request->producer.modes = modes[i].modes;
        }
    }
    if (sz == NULL) {
        request->sz = SHCKIT_SZ_UNKNOWN;
    } else if (strcmp(sz, "0") == 0) {
        request->sz = SHCKIT_SZ_0;
    } else if (strcmp(sz, "1") == 0) {
        request->sz = SHCKIT_SZ_1;
    } else {
        ok = false;
    }
    if (!ok || request->in == NULL || request->producer.release == NULL ||
        request->producer.modes == 0 || request->entry == NULL) {
        fputs(usage, stderr);
        return false;
    }
    return true;
}

static bool find_label(const char *path, const struct shckit_text *text,
                       const char *name, size_t *index)
{
    // the line that defines the label, which has to be the only one
    size_t len = strlen(name);
    *index = SIZE_MAX;
    for (size_t i = 0; i < shckit_count(text); i++) {
        const struct shckit_line *line = shckit_line(text, i);
        if (line->label.len == 0 || line->label.len != len ||
            memcmp(line->bytes + line->label.at, name, len) != 0) {
            continue;
        }
        if (*index != SIZE_MAX) {
            fprintf(stderr, "shckit: %s: %s is defined twice\n", path, name);
            return false;
        }
        *index = i;
    }
    if (*index == SIZE_MAX) {
        fprintf(stderr, "shckit: %s: no label is called %s\n", path, name);
        return false;
    }
    return true;
}

static size_t block_end(const struct shckit_text *text, size_t first)
{
    // the line that ends the entry's .SECTION block: the next .SECTION or
    // .END line, or the text's end
    size_t end = first + 1;
    while (end < shckit_count(text) &&
           shckit_line(text, end)->directive != SHCKIT_SECTION &&
           shckit_line(text, end)->directive != SHCKIT_END) {
        end++;
    }
    return end;
}

static int print_inspection(const struct inspect_request *request,
                            const struct shckit_text *text,
                            const struct shckit_function *function,
                            const struct shckit_inspection *result,
                            const struct shckit_witness *witnesses,
                            size_t count)
{
    // each witness, or none, then the limit. none, and a limit with no line
    // of its own, name the entry's.
    static const char *const kinds[] = {"none", "opcode", "operand", "abi"};

    for (size_t i = 0; i < count; i++) {
        const struct shckit_witness *each = &witnesses[i];
        printf("%s:%zu: %s: %s: %s", request->in,
               shckit_line(text, each->index)->input, request->entry,
               kinds[each->kind], each->rule);
        if (each->kind == SHCKIT_ABI && each->value != 0) {
            printf(" (SP %+" PRId32 ")", each->value);
        } else if (each->kind == SHCKIT_OPERAND &&
                   strcmp(each->rule, "small-constant") == 0) {
            printf(" (#%" PRId32 ")", each->value);
        }
        putchar('\n');
    }
    if (count == 0) {
        printf("%s:%zu: %s: %s\n", request->in,
               shckit_line(text, function->first)->input, request->entry,
               kinds[SHCKIT_NO_WITNESS]);
    }
    if (result->incomplete != NULL) {
        size_t at = result->incomplete_index != SIZE_MAX
                        ? result->incomplete_index
                        : function->first;
        printf("%s:%zu: %s: incomplete: %s\n", request->in,
               shckit_line(text, at)->input, request->entry,
               result->incomplete);
    }
    errno = 0;
    if (fflush(stdout) != 0 || ferror(stdout) != 0) {
        complain("stdout");
        return FAILED;
    }
    return result->incomplete != NULL ? UNFINISHED : OK;
}

static int inspect_file(int argc, char **argv)
{
    // a text check refuses gets check's findings, not an inspection
    struct inspect_request request = {{NULL, 0}, NULL, NULL, NULL, 0, false};
    struct shckit_function function = {0, 0, SHCKIT_SZ_UNKNOWN};
    struct shckit_inspection result;
    struct shckit_witness *witnesses = NULL;
    size_t count = 0;
    struct shckit_text *text = NULL;
    int error = SHCKIT_OK;
    int status = OK;

    if (!read_inspect_request(argc, argv, &request)) {
        return FAILED;
    }
    status = load(request.in, &text);
    if (status == OK) {
        status = report(request.in, text, shckit_check);
    }
    if (status == OK &&
        !find_label(request.in, text, request.entry, &function.first)) {
        status = FAILED;
    }
    if (status == OK && request.end == NULL) {
        function.end = block_end(text, function.first);
    } else if (status == OK &&
               !find_label(request.in, text, request.end, &function.end)) {
        status = FAILED;
    }
    if (status == OK) {
        function.sz = request.sz;
        if (request.all) {
            error = shckit_inspect_all(text, &request.producer, &function,
                                       &result, &witnesses, &count);
        } else {
            error = shckit_inspect(text, &request.producer, &function, &result);
        }
        if (error != SHCKIT_OK) {
            fprintf(stderr, "shckit: %s: %s\n", request.in,
                    shckit_strerror(error));
            status = FAILED;
        } else if (request.all) {
            status = print_inspection(&request, text, &function, &result,
                                      witnesses, count);
        } else {
            status = print_inspection(
                &request, text, &function, &result, &result.witness,
                result.witness.kind != SHCKIT_NO_WITNESS ? 1 : 0);
        }
    }
    free(witnesses);
    shckit_free(text);
    return status;
}

int main(int argc, char **argv)
{
    const char *command = argc > 1 ? argv[1] : "";

    if (strcmp(command, "apply") == 0) {
        return rewrite_file(argc, argv, true);
    }
    if (argc > 2 && strcmp(command, "check") == 0) {
        return check_files(argc, argv, shckit_check);
    }
    if (argc > 2 && strcmp(command, "lint") == 0) {
        return check_files(argc, argv, shckit_lint);
    }
    if (strcmp(command, "inspect") == 0) {
        return inspect_file(argc, argv);
    }
    if (strcmp(command, "translate") == 0) {
        return rewrite_file(argc, argv, false);
    }
    fputs(usage, stderr);
    return FAILED;
}
