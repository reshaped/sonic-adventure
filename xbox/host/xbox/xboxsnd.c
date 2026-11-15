/* xboxsnd OUT.pak DIR [-list]: the Xbox's sound pack, from the disc's sound
 * banks under DIR.
 *
 * a bank file, .MLT, is a table of the chunks the sound library loads into
 * sound RAM, each at an address of its own: program banks (SMPB), sequence
 * banks (SMSB) and the effects' (SFPB, SFOB). a program bank is its
 * programs, each up to four layers of splits, and a split is the AICA's
 * registers for a sample as the driver writes them: the format and the loop
 * flag with the sample's address from the bank's start, the loop's start
 * and end. the samples follow, 16-bit and 8-bit PCM and the AICA's 4-bit
 * ADPCM, which the Xbox's sound chip doesn't play: the host's bridge
 * (aica_apu.c) decodes one, or turns an 8-bit one unsigned, as a channel
 * first keys it on.
 *
 * this does it ahead: every split's sample that isn't 16-bit PCM, by bank
 * file, as the bridge would convert it (the decoder is Flycast's, sgc_if.cpp's
 * DecodeADPCM, so the samples are the same). the host reads a bank file's
 * part of the pack as the game opens that file, and a key on then finds its
 * sample ready, by its address in sound RAM, its length and its first bytes.
 *
 * the pack: a header (magic, the bank files' count), an entry a bank file
 * (name, offset, samples, bytes), and at each offset the file's records
 * (struct sndpack_sample) then their samples */
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "sndpack.h"

static int list;

static uint32_t u32(const uint8_t *d, size_t at)
{
    return d[at] | d[at + 1] << 8 | d[at + 2] << 16 | (uint32_t)d[at + 3] << 24;
}

static uint32_t u16(const uint8_t *d, size_t at)
{
    return d[at] | d[at + 1] << 8;
}

/* Flycast's DecodeADPCM, count samples from d, the low nibble first */
static void adpcm(const uint8_t *d, uint32_t count, int16_t *out)
{
    static const int32_t qs[8] = {0x0e6, 0x0e6, 0x0e6, 0x0e6, 0x133, 0x199, 0x200, 0x266};
    static const int32_t scale[16] = {1, 3, 5, 7, 9, 11, 13, 15, -1, -3, -5, -7, -9, -11, -13, -15};
    int32_t prev = 0, quant = 127;
    uint32_t k;

    for (k = 0; k < count; k++) {
        uint32_t sample = (d[k >> 1] >> ((k & 1) * 4)) & 15;
        int32_t sign = 1 - 2 * (int32_t)(sample / 8);
        uint32_t data = sample & 7;
        int32_t rv = (quant * scale[data]) >> 3;

        if (rv > 0x7fff) rv = 0x7fff;
        rv = sign * rv + prev;
        quant = (quant * qs[data]) >> 8;
        if (quant < 127) quant = 127;
        if (quant > 24576) quant = 24576;
        if (rv < -32768) rv = -32768;
        if (rv > 32767) rv = 32767;
        prev = rv;
        out[k] = (int16_t)rv;
    }
}

struct file {
    struct sndpack_file entry;
    struct sndpack_sample *samples;
    uint8_t *data;
    size_t count, cap, bytes, data_cap;
};

static void add(struct file *f, uint32_t sa, uint32_t count, uint32_t pcms, const uint8_t *from)
{
    struct sndpack_sample *s;
    size_t bytes = pcms == 1 ? count : count * 2, k;

    for (k = 0; k < f->count; k++) {
        if (f->samples[k].sa == sa && f->samples[k].count == count && f->samples[k].pcms == pcms) return;
    }
    if (f->count == f->cap) f->samples = realloc(f->samples, (f->cap = f->cap ? f->cap * 2 : 64) * sizeof *f->samples);
    while (f->bytes + bytes + 4 > f->data_cap) f->data = realloc(f->data, f->data_cap = f->data_cap ? f->data_cap * 2 : 1 << 20);
    s = &f->samples[f->count++];
    s->sa = sa;
    s->count = count;
    s->pcms = pcms;
    memcpy(s->head, from, sizeof s->head);
    s->data = (uint32_t)f->bytes;
    s->bytes = (uint32_t)bytes;
    if (pcms == 1) {
        for (k = 0; k < count; k++) f->data[f->bytes + k] = from[k] ^ 0x80;
    } else {
        adpcm(from, count, (int16_t *)(void *)(f->data + f->bytes));
    }
    f->bytes += (bytes + 3) & ~(size_t)3;
}

/* a program bank at d, loaded at ram: its splits' samples */
static void bank(struct file *f, const uint8_t *d, size_t size, uint32_t ram, const char *name)
{
    uint32_t table, programs, p, l, s;

    if (size < 0x18 || memcmp(d, "SMPB", 4) != 0) return;
    table = u32(d, 0x10);
    programs = u32(d, 0x14);
    for (p = 0; p < programs && table + 4 * p + 4 <= size; p++) {
        uint32_t program = u32(d, table + 4 * p);

        if (program == 0 || program + 16 > size) continue;
        for (l = 0; l < 4; l++) {
            uint32_t layer = u32(d, program + 4 * l), splits, at;

            if (layer == 0 || layer + 8 > size) continue;
            splits = u32(d, layer);
            at = u32(d, layer + 4);
            if (splits > 128 || at + splits * 0x30 > size) continue;
            for (s = 0; s < splits; s++) {
                uint32_t r0 = u16(d, at + s * 0x30), sa = (r0 & 0x7f) << 16 | u16(d, at + s * 0x30 + 2);
                uint32_t lea = u16(d, at + s * 0x30 + 6), pcms = (r0 >> 7) & 3;
                /* as the bridge plays it: to LEA, a loop's end too */
                uint32_t count = lea + 1, bytes = pcms == 1 ? count : (count + 1) / 2;

                if (pcms == 0 || sa + bytes > size || count < 2) continue;
                add(f, ram + sa, count, pcms == 3 ? 2 : pcms, d + sa);
                if (list) printf("%s bank at %06x: sample %06x, %u samples, format %u\n", name, ram, ram + sa, count, pcms);
            }
        }
    }
}

static int by_name(const struct dirent **a, const struct dirent **b)
{
    return strcasecmp((*a)->d_name, (*b)->d_name);
}

int main(int argc, char **argv)
{
    struct sndpack_header header = {SNDPACK_MAGIC, 0};
    struct file *files = NULL;
    struct dirent **names;
    size_t count = 0, samples = 0, bytes = 0, k;
    uint32_t at;
    FILE *out;
    int n, i;

    if (argc < 3) {
        fprintf(stderr, "usage: xboxsnd OUT.pak DIR [-list]\n");
        return 2;
    }
    list = argc > 3 && strcmp(argv[3], "-list") == 0;
    n = scandir(argv[2], &names, NULL, by_name);
    for (i = 0; i < n; i++) {
        const char *dot = strrchr(names[i]->d_name, '.');
        char path[1024];
        uint8_t *d;
        long size;
        uint32_t chunks, c;
        struct file *f;
        FILE *in;

        if (dot == NULL || strcasecmp(dot, ".MLT") != 0 || strlen(names[i]->d_name) >= sizeof f->entry.name) continue;
        snprintf(path, sizeof path, "%s/%s", argv[2], names[i]->d_name);
        in = fopen(path, "rb");
        if (in == NULL) continue;
        fseek(in, 0, SEEK_END);
        size = ftell(in);
        rewind(in);
        d = malloc((size_t)size + 16);
        if (fread(d, 1, (size_t)size, in) != (size_t)size) size = 0;
        fclose(in);
        if (size < 0x20 || memcmp(d, "SMLT", 4) != 0) {
            free(d);
            continue;
        }
        files = realloc(files, (count + 1) * sizeof *files);
        f = &files[count++];
        memset(f, 0, sizeof *f);
        for (c = 0; names[i]->d_name[c] != '\0'; c++) {
            char ch = names[i]->d_name[c];

            f->entry.name[c] = ch >= 'a' && ch <= 'z' ? (char)(ch - 'a' + 'A') : ch;
        }
        chunks = u32(d, 8);
        for (c = 0; c < chunks && 0x40 + c * 0x20 <= (size_t)size; c++) {
            const uint8_t *e = d + 0x20 + c * 0x20;
            uint32_t ram = u32(e, 8), offset = u32(e, 16), length = u32(e, 20);

            if (memcmp(e, "SMPB", 4) == 0 && offset != 0xffffffffu && offset + length <= (size_t)size) {
                bank(f, d + offset, length, ram, f->entry.name);
            }
        }
        free(d);
        samples += f->count;
        bytes += f->bytes;
    }
    out = fopen(argv[1], "wb");
    if (out == NULL) {
        perror(argv[1]);
        return 1;
    }
    header.count = (uint32_t)count;
    fwrite(&header, sizeof header, 1, out);
    at = (uint32_t)(sizeof header + count * sizeof(struct sndpack_file));
    for (k = 0; k < count; k++) {
        files[k].entry.offset = at;
        files[k].entry.samples = (uint32_t)files[k].count;
        files[k].entry.bytes = (uint32_t)(files[k].count * sizeof(struct sndpack_sample) + files[k].bytes);
        at += files[k].entry.bytes;
        fwrite(&files[k].entry, sizeof files[k].entry, 1, out);
    }
    for (k = 0; k < count; k++) {
        fwrite(files[k].samples, sizeof(struct sndpack_sample), files[k].count, out);
        fwrite(files[k].data, 1, files[k].bytes, out);
    }
    fclose(out);
    fprintf(stderr, "xboxsnd: %zu bank files, %zu samples, %zu MB\n", count, samples, (bytes + (1 << 19)) >> 20);
    return 0;
}
