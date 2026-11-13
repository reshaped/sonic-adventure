/* the settings file: a line is a name and its value, the rest of the line.
 * # begins a comment. the runner takes disc, and play takes its own
 * names. the backend's pads take the rest (engine/pc/input.h), and refuse a
 * name nobody took. a player's settings lie in sonic-adventure/ under
 * $XDG_CONFIG_HOME, else ~/.config. the card lies in sonic-adventure/ under
 * $XDG_DATA_HOME, else ~/.local/share. on Windows both lie in
 * %APPDATA%\sonic-adventure\ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native.h"

#define LINE 4096

/* out, where snprintf's length says the whole path fit, else null. a path
 * cut short names another file */
static const char *whole(const char *out, size_t size, int length)
{
    return length >= 0 && (size_t)length < size ? out : NULL;
}

#ifdef _WIN32
const char *native_settings_place(char *out, size_t size, int data,
                                  const char *name)
{
    const char *base = getenv("APPDATA");

    (void)data;
    if (!base || !base[0]) {
        return NULL;
    }
    return whole(out, size,
                 snprintf(out, size, "%s\\sonic-adventure\\%s", base, name));
}
#else
const char *native_settings_place(char *out, size_t size, int data,
                                  const char *name)
{
    const char *base = getenv(data ? "XDG_DATA_HOME" : "XDG_CONFIG_HOME");
    const char *home = getenv("HOME");

    /* a base that isn't absolute is none, as XDG says */
    if (base && base[0] == '/') {
        return whole(out, size,
                     snprintf(out, size, "%s/sonic-adventure/%s", base, name));
    }
    if (home && home[0]) {
        return whole(out, size,
                     snprintf(out, size, "%s/%s/sonic-adventure/%s", home,
                              data ? ".local/share" : ".config", name));
    }
    return NULL;
}
#endif

char *native_settings_read(const char *path)
{
    FILE *in = fopen(path, "rb");
    char *text = NULL;
    long bytes = -1;

    if (in && !fseek(in, 0, SEEK_END) && (bytes = ftell(in)) >= 0 &&
        !fseek(in, 0, SEEK_SET) && (text = calloc(1, (size_t)bytes + 1)) &&
        fread(text, 1, (size_t)bytes, in) != (size_t)bytes) {
        free(text);
        text = NULL;
    }
    if (in) {
        fclose(in);
    }
    return text;
}

void native_settings_take(char *text, const char *path,
                          int (*take)(const char *name, const char *value))
{
    int number = 1;

    for (char *line = text; *line; number++) {
        size_t length = strcspn(line, "\n");
        char copy[LINE];
        char *name, *value, *end;
        int took;

        snprintf(copy, sizeof copy, "%.*s", (int)length, line);
        copy[strcspn(copy, "#")] = 0;
        name = copy + strspn(copy, " \t\r");
        value = name + strcspn(name, " \t\r");
        if (*value) {
            *value++ = 0;
        }
        value += strspn(value, " \t\r");
        end = value + strlen(value);
        while (end > value && strchr(" \t\r", end[-1])) {
            end--;
        }
        *end = 0;
        if (*name && (took = take(name, value)) != 0) {
            if (took < 0) {
                fprintf(stderr, "sa1: %s, line %d: %s won't take \"%s\"\n",
                        path, number, name, value);
                exit(2);
            }
            /* blank, so the backend's lines keep their numbers */
            memset(line, ' ', length);
        }
        line += length + (line[length] == '\n');
    }
}
