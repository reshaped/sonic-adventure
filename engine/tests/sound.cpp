/* checks of the PC backend's sound, on bytes made here. its streams are
 * checked against the game's own decoder, ADX_DecodeSte2, linked from
 * sdk/sofdec. load and stream copy their bytes at the call. the checks clear
 * the caller's bytes after each call to show this. main returns 0, 99 where
 * create fails, or the number of the first check that fails */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

#include "sound.h"

extern "C" int32_t ADX_DecodeSte2(int8_t *ibuf, int32_t nblk, int16_t *obufl,
                                  int16_t *delayl, int16_t *obufr,
                                  int16_t *delayr, int16_t k0, int16_t k1);

namespace {

constexpr uint32_t PAIRS = 64; /* an ADX's blocks, each channel's */
constexpr uint32_t BANK = 0x10000; /* where the banks start in sound memory */

void put16(std::vector<uint8_t> &b, uint32_t at, uint32_t v)
{
    b[at] = static_cast<uint8_t>(v);
    b[at + 1] = static_cast<uint8_t>(v >> 8);
}

void put32(std::vector<uint8_t> &b, uint32_t at, uint32_t v)
{
    put16(b, at, v & 0xffff);
    put16(b, at + 2, v >> 16);
}

void big(std::vector<uint8_t> &b, uint32_t at, uint32_t v, int bytes)
{
    for (int i = 0; i < bytes; i++) {
        b[at + i] = static_cast<uint8_t>(v >> 8 * (bytes - 1 - i));
    }
}

/* a stereo ADX at 44.1 kHz with no loop. its PAIRS pairs of blocks hold
 * pseudo-random codes, loud enough that some samples clamp */
std::vector<uint8_t> adx()
{
    std::vector<uint8_t> b(0x34 + 36 * PAIRS + 18, 0);

    big(b, 0, 0x8000, 2);
    big(b, 2, 0x30, 2); /* the blocks from 0x34 */
    b[4] = 3;
    b[5] = 18; /* bytes a block */
    b[6] = 4; /* bits a code */
    b[7] = 2; /* channels */
    big(b, 8, 44100, 4);
    big(b, 12, 32 * PAIRS, 4);
    big(b, 16, 500, 2); /* the filter's cutoff, in Hz */
    b[0x12] = 3; /* version 3, whose count of loops at 0x16 is 0 */
    uint32_t seed = 1;

    for (uint32_t i = 0; i < 2 * PAIRS; i++) {
        uint32_t at = 0x34 + 18 * i;

        big(b, at, (i * 977) & 0x1fff, 2);
        for (uint32_t j = 2; j < 18; j++) {
            seed = seed * 1103515245 + 12345;
            b[at + j] = static_cast<uint8_t>(seed >> 16);
        }
    }
    big(b, 0x34 + 36 * PAIRS, 0x8001, 2); /* the end mark */
    return b;
}

/* adx() with a header that holds a loop, and its blocks from offset. the
 * loop's count of loops is count, its type is 1, and it runs from sample 256
 * to sample 768 */
std::vector<uint8_t> looped(uint32_t count, uint32_t offset)
{
    std::vector<uint8_t> plain = adx();
    std::vector<uint8_t> b(offset, 0);

    memcpy(b.data(), plain.data(), 0x13);
    b.insert(b.end(), plain.begin() + 0x34, plain.end());
    big(b, 2, offset - 4, 2);
    big(b, 0x16, count, 2);
    big(b, 0x1a, 1, 2);
    big(b, 0x1c, 256, 4);
    big(b, 0x24, 768, 4);
    return b;
}

/* a program bank of seven programs, their table at 0x5e0. program 0 has one
 * layer of one split, over keys 48 to 72 with its own note 60. it plays 400
 * samples of 16-bit 0x4000 from the bank's 0x100 */
std::vector<uint8_t> programs()
{
    std::vector<uint8_t> b(0xc00, 0);

    memcpy(b.data(), "SMPB", 4);
    put32(b, 4, 1);
    put32(b, 8, static_cast<uint32_t>(b.size()));
    put32(b, 0x10, 0x5e0); /* the programs' table */
    put32(b, 0x14, 7);
    put32(b, 0x5e0, 0x40); /* program 0 */
    put32(b, 0x40, 0x58); /* its layer 0 */
    put32(b, 0x58, 1); /* one split */
    put32(b, 0x5c, 0x68);
    b[0x64] = 2; /* its bend's range */
    uint32_t s = 0x68;

    put16(b, s + 0, 0x0000); /* 16-bit, no loop, SA's top bits 0 */
    put16(b, s + 2, 0x0100); /* SA */
    put16(b, s + 4, 0);
    put16(b, s + 6, 400); /* LEA */
    put16(b, s + 8, 0x001f); /* AR 31 */
    put16(b, s + 10, 0x001f | 15 << 10); /* RR 31, no key scale */
    put16(b, s + 18, 0x0f00); /* DISDL 15, DIPAN 0 */
    put16(b, s + 20, 0x0020); /* TL 0, LPOFF */
    b[s + 36] = 48;
    b[s + 37] = 72;
    b[s + 38] = 60;
    for (uint32_t i = 0; i < 400; i++) {
        put16(b, 0x100 + 2 * i, 0x4000);
    }
    /* program 1: program 0's split in ADPCM from the bank's 0x500. its 22
     * codes are 20 of +7, then -7 and 0 */
    put32(b, 0x5e4, 0x500 + 16);
    put32(b, 0x500 + 16, 0x500 + 32);
    put32(b, 0x500 + 32, 1);
    put32(b, 0x500 + 36, 0x500 + 48);
    memcpy(b.data() + 0x500 + 48, b.data() + s, 0x30);
    put16(b, 0x500 + 48, 2 << 7); /* ADPCM */
    put16(b, 0x500 + 50, 0x500);
    put16(b, 0x500 + 54, 22);
    for (uint32_t i = 0; i < 10; i++) {
        b[0x500 + i] = 0x77;
    }
    b[0x500 + 10] = 0x0f;
    /* programs 2 to 4: program 0's split from 0x600, 0x650 and 0x6a0.
     * program 2 sends all of it to the DSP's input 0 and none direct.
     * program 3's square LFO takes 24 dB off the first half of each 256
     * frames. program 4 is filtered at a level of 0x1600 */
    for (uint32_t k = 0; k < 3; k++) {
        uint32_t at = 0x600 + 0x50 * k;

        put32(b, 0x5e8 + 4 * k, at);
        put32(b, at, at + 0x10);
        put32(b, at + 0x10, 1);
        put32(b, at + 0x14, at + 0x18);
        memcpy(b.data() + at + 0x18, b.data() + s, 0x30);
    }
    put16(b, 0x618 + 16, 0x00f0); /* IMXL 15, ISEL 0 */
    /* program 5: program 2's split, sending to input 1, from 0x700 */
    put32(b, 0x5f4, 0x700);
    put32(b, 0x700, 0x710);
    put32(b, 0x710, 1);
    put32(b, 0x714, 0x718);
    memcpy(b.data() + 0x718, b.data() + 0x618, 0x30);
    put16(b, 0x718 + 16, 0x00f1); /* IMXL 15, ISEL 1 */
    put16(b, 0x718 + 18, 0x0000); /* DISDL 0 */
    put16(b, 0x618 + 18, 0x0000); /* DISDL 0 */
    put16(b, 0x668 + 14, 31 << 10 | 1 << 3 | 7); /* LFOF 31, square, 24 dB */
    put16(b, 0x6b8 + 20, 0x0004); /* LPOFF clear, Q 4 */
    for (uint32_t k = 0; k < 5; k++) {
        put16(b, 0x6b8 + 22 + 2 * k, 0x1600);
    }
    /* program 6: program 0's split from 0xb40, under a filter held at
     * 0x1ff8. its 400 samples at 0x800 alternate 0x4000 and -0x4000, a tone
     * at half the rate */
    put32(b, 0x5f8, 0xb40);
    put32(b, 0xb40, 0xb50);
    put32(b, 0xb50, 1);
    put32(b, 0xb54, 0xb58);
    memcpy(b.data() + 0xb58, b.data() + s, 0x30);
    put16(b, 0xb58 + 2, 0x0800); /* SA */
    put16(b, 0xb58 + 20, 0x0004); /* LPOFF clear, Q 4 */
    for (uint32_t k = 0; k < 5; k++) {
        put16(b, 0xb58 + 22 + 2 * k, 0x1ff8);
    }
    for (uint32_t i = 0; i < 400; i++) {
        put16(b, 0x800 + 2 * i, i & 1 ? 0xc000 : 0x4000);
    }
    /* a table of levels at 0x780: 127 but for volume 106's 115. volume 106
     * takes 24 steps of total level, 9 dB, where a bank without a table
     * takes 12 */
    put32(b, 0x18, 0x780);
    put32(b, 0x1c, 1);
    memset(b.data() + 0x780, 127, 128);
    b[0x780 + 106] = 115;
    return b;
}

void step(std::vector<uint8_t> &b, uint32_t image, uint32_t i, uint32_t w0,
          uint32_t w1, uint32_t w2, uint32_t w3)
{
    uint32_t at = image + 0x400 + 16 * i;

    put32(b, at, w0);
    put32(b, at + 4, w1);
    put32(b, at + 8, w2);
    put32(b, at + 12, w3);
}

/* an fx bank of one program, whose step n takes coefficient n. step 0 takes
 * input 0 at a coefficient of 4095. step 1 writes it to output 0 and to the
 * ring. step 3 reads the ring 100 words on, and step 5 takes the read. steps
 * 6 and 7 write it to output 1 at a coefficient of 4095. so output 1 is
 * output 0 100 samples late. steps 84 and 85 write input 1 to output 2 at
 * coefficient 84. it is 0 until qsound sets it */
std::vector<uint8_t> fx_programs()
{
    constexpr uint32_t IMAGE = 0x40;
    std::vector<uint8_t> b(IMAGE + 0xc00 + 4, 0);

    memcpy(b.data(), "SFPB", 4);
    put32(b, 4, 1);
    put32(b, 8, static_cast<uint32_t>(b.size()));
    put32(b, 12, 1);
    put32(b, 0x10, 0x14); /* program 0's entry */
    put32(b, 0x14 + 0x24, IMAGE); /* its registers' image */
    put32(b, IMAGE + 0, 0x7ff8);
    put32(b, IMAGE + 4 * 6, 0x7ff8);
    put32(b, IMAGE + 0x200 + 4, 100); /* MADRS 1 */
    step(b, IMAGE, 0, 0, 0xb000, 0x0002, 0); /* XSEL YSEL IRA 0x20, ZERO */
    step(b, IMAGE, 1, 0, 0, 0x5000, 0); /* MWT, EWT to 0 */
    step(b, IMAGE, 3, 0, 0, 0x2000, 0x0200); /* MRD, MASA 1 */
    step(b, IMAGE, 5, 0, 0x0040, 0, 0); /* IWT to 0 */
    step(b, IMAGE, 6, 0, 0xa000, 0x0002, 0); /* XSEL YSEL IRA 0, ZERO */
    step(b, IMAGE, 7, 0, 0, 0x1100, 0); /* EWT to 1 */
    step(b, IMAGE, 84, 0, 0xb080, 0x0002, 0); /* XSEL YSEL IRA 0x21, ZERO */
    step(b, IMAGE, 85, 0, 0, 0x1200, 0); /* EWT to 2 */
    return b;
}

/* a stand-in for the game's driver at sound memory's start. its version at
 * 0x20 is 1.01 build 0x38, the build whose qsound table the backend reads.
 * in that table at 0x3bf0, place 4's row sends 4095 to the first mix alone */
std::vector<uint8_t> driver()
{
    std::vector<uint8_t> b(0x3bf0 + 512, 0);

    memcpy(b.data() + 0x20, "\x01\x01\x38\x00 GM ", 8);
    put16(b, 0x3bf0 + 16, 0x7ff8);
    return b;
}

/* an fx outputs bank: output 0 all left, output 1 all right and output 2
 * all left */
std::vector<uint8_t> fx_outputs()
{
    std::vector<uint8_t> b(0x3c, 0);
    const uint8_t set[] = {15, 0x1f, 15, 0x0f, 15, 0x1f};

    memcpy(b.data(), "SFOB", 4);
    put32(b, 4, 1);
    put32(b, 8, static_cast<uint32_t>(b.size()));
    put32(b, 12, 1);
    put32(b, 0x10, 0x18);
    memcpy(b.data() + 0x18, set, sizeof set);
    return b;
}

/* a sequence bank of one sequence at tempo 800, a tick a millisecond. at
 * tick 1 it takes bank 0's program 0 and plays key 60 for 100 ticks. it plays
 * key 60 again for 50 on the tick the first one's gate ends */
std::vector<uint8_t> sequences()
{
    std::vector<uint8_t> b(0x60, 0);
    const uint8_t events[] = {0x84, 0x03, 0x20, 0x00, 0xb0, 0x20, 0x00, 0x01,
                              0xc0, 0x00, 0x00, 0x00, 0x3c, 0x7f, 0x64, 0x64,
                              0x00, 0x3c, 0x7f, 0x32, 0x00, 0x83, 0x00, 0x00};

    memcpy(b.data(), "SMSB", 4);
    put32(b, 4, 1);
    put32(b, 8, static_cast<uint32_t>(b.size()));
    put32(b, 12, 1);
    put32(b, 16, 0x20);
    memcpy(b.data() + 0x20, "SMSD", 4);
    put32(b, 0x24, 0x88);
    put32(b, 0x28, 800);
    memcpy(b.data() + 0x2c, events, sizeof events);
    return b;
}

sa_audio_command midi(uint32_t port, uint32_t status, uint32_t d1, uint32_t d2)
{
    sa_audio_command c{};

    c.kind = SA_AUDIO_MIDI;
    c.key.id = port;
    c.value = static_cast<int32_t>(status | d1 << 8 | d2 << 16);
    return c;
}

/* render a frame at a time until the left side is 0. it returns the frames
 * before then, at most 4001. first takes the first frame's left */
uint32_t heard(pc::Sound &sound, int16_t &first)
{
    int16_t frame[2];
    uint32_t n = 0;

    for (;; n++) {
        sound.render(frame, 1);
        if (n == 0) {
            first = frame[0];
        }
        if (frame[0] == 0 || n > 4000) {
            return n;
        }
    }
}

/* play file on stream 0 for adx()'s samples and 8 frames more, and stop it.
 * it returns whether the stream played as looping says. looping, it plays to
 * sample 768, and then from 256 to there over and over. otherwise it plays
 * each sample its header counts once and ends. left and right are adx()'s
 * samples */
bool plays(pc::Sound &sound, std::vector<uint8_t> file, bool looping,
           const std::vector<int16_t> &left, const std::vector<int16_t> &right)
{
    std::vector<int16_t> out(2 * (32 * PAIRS + 8));
    uint32_t samples = static_cast<uint32_t>(file[12] << 24 | file[13] << 16 |
                                             file[14] << 8 | file[15]);
    sa_audio_command stop{};
    bool as_said = true;

    stop.kind = SA_AUDIO_STREAM_STOP;
    if (sound.stream(0, file.data(), static_cast<uint32_t>(file.size())) !=
        SA_OK) {
        return false;
    }
    sound.render(out.data(), 32 * PAIRS + 8);
    sound.submit(&stop, 1);
    for (uint32_t i = 0; i < 32 * PAIRS + 8; i++) {
        uint32_t at = looping && i >= 768 ? 256 + (i - 768) % 512 : i;
        int16_t l = at < samples ? left[at] : 0;
        int16_t r = at < samples ? right[at] : 0;

        as_said = as_said && out[2 * i] == l && out[2 * i + 1] == r;
    }
    return as_said;
}

/* a stream's loop, as sa_audio_stream says a file holds one: a count of
 * loops of 1, in a header of 48 bytes or more, around samples the file
 * holds */
int loops(pc::Sound &sound)
{
    std::vector<uint8_t> file = adx();
    std::vector<int16_t> left(32 * PAIRS), right(32 * PAIRS);
    int16_t delay_l[2] = {0, 0}, delay_r[2] = {0, 0};

    ADX_DecodeSte2(reinterpret_cast<int8_t *>(file.data() + 0x34), 2 * PAIRS,
                   left.data(), delay_l, right.data(), delay_r, 7334, -3283);
    /* a count of 1 loops, whatever the type at 0x1a says */
    std::vector<uint8_t> untyped = looped(1, 0x34);

    big(untyped, 0x1a, 0, 2);
    if (!plays(sound, looped(1, 0x34), true, left, right) ||
        !plays(sound, untyped, true, left, right)) {
        return 60;
    }
    /* a count of loops other than 1 is no loop, whatever the type says */
    if (!plays(sound, looped(0, 0x34), false, left, right) ||
        !plays(sound, looped(2, 0x34), false, left, right)) {
        return 61;
    }
    /* a header of 44 bytes is too short to hold a loop, though its words
     * are there */
    if (!plays(sound, looped(1, 0x2c), false, left, right)) {
        return 62;
    }
    /* the loop's words lie where they do whatever version the header
     * gives */
    const uint8_t versions[3] = {2, 4, 5};

    for (uint8_t version : versions) {
        std::vector<uint8_t> other = looped(1, 0x34);

        other[0x12] = version;
        if (!plays(sound, other, true, left, right)) {
            return 63;
        }
    }
    /* a loop that ends at or before its first sample is none. nor is one
     * that ends past the samples the header counts, though the file's
     * blocks run on */
    std::vector<uint8_t> back = looped(1, 0x34), past = looped(1, 0x34);

    big(back, 0x24, 256, 4);
    big(past, 12, 512, 4);
    if (!plays(sound, back, false, left, right) ||
        !plays(sound, past, false, left, right)) {
        return 64;
    }
    return 0;
}

int check(pc::Sound &sound)
{
    /* ADX: the backend's stream is the game's decoder's to the bit, past
     * clamps and to the end mark. 7334 and -3283 are what ADX_GetCoefficient
     * gives the header's cutoff at 44.1 kHz */
    std::vector<uint8_t> file = adx();
    std::vector<int16_t> left(32 * PAIRS), right(32 * PAIRS);
    int16_t delay_l[2] = {0, 0}, delay_r[2] = {0, 0};

    ADX_DecodeSte2(reinterpret_cast<int8_t *>(file.data() + 0x34), 2 * PAIRS,
                   left.data(), delay_l, right.data(), delay_r, 7334, -3283);
    if (sound.stream(0, file.data(), static_cast<uint32_t>(file.size())) !=
        SA_OK) {
        return 1;
    }
    memset(file.data(), 0, file.size());
    std::vector<int16_t> out(2 * (32 * PAIRS + 8));
    int clamped = 0;

    sound.render(out.data(), 32 * PAIRS + 8);
    for (uint32_t i = 0; i < 32 * PAIRS; i++) {
        clamped += left[i] == 32767 || left[i] == -32768;
        if (out[2 * i] != left[i] || out[2 * i + 1] != right[i]) {
            fprintf(stderr, "sound: frame %u is %d %d, the game's %d %d\n", i,
                    out[2 * i], out[2 * i + 1], left[i], right[i]);
            return 2;
        }
    }
    if (clamped == 0 || out[2 * 32 * PAIRS] != 0) {
        return 3;
    }
    /* a stream of 17 MB plays from its start as a short one does */
    std::vector<uint8_t> song = adx();
    uint32_t blocks = (17U << 20) / 36;
    sa_audio_command stop{};

    song.resize(0x34 + 36 * blocks + 18, 0);
    big(song, 12, 32 * blocks, 4);
    big(song, 0x34 + 36 * blocks, 0x8001, 2);
    stop.kind = SA_AUDIO_STREAM_STOP;
    if (sound.stream(0, song.data(), static_cast<uint32_t>(song.size())) !=
        SA_OK) {
        return 24;
    }
    sound.render(out.data(), 32 * PAIRS);
    sound.submit(&stop, 1);
    for (uint32_t i = 0; i < 32 * PAIRS; i++) {
        if (out[2 * i] != left[i] || out[2 * i + 1] != right[i]) {
            return 25;
        }
    }
    /* told to wait 10 ms, a stream is silent for 441 frames, then plays
     * from its start */
    sa_audio_command wait{};
    std::vector<int16_t> late(2 * (441 + 32 * PAIRS));

    wait.kind = SA_AUDIO_STREAM_WAIT;
    wait.value = 10;
    sound.submit(&wait, 1);
    sound.stream(0, song.data(), static_cast<uint32_t>(song.size()));
    sound.render(late.data(), 441 + 32 * PAIRS);
    sound.submit(&stop, 1);
    for (uint32_t i = 0; i < 441 + 32 * PAIRS; i++) {
        int16_t l = i < 441 ? 0 : left[i - 441];
        int16_t r = i < 441 ? 0 : right[i - 441];

        if (late[2 * i] != l || late[2 * i + 1] != r) {
            fprintf(stderr, "sound: a late stream's frame %u is %d %d\n", i,
                    late[2 * i], late[2 * i + 1]);
            return 28;
        }
    }

    /* a note: bank 0's program 0 on port 0's channel 3 */
    std::vector<uint8_t> bank = programs();
    sa_audio_bank place = {SA_BANK_PROGRAMS, 0, BANK,
                           static_cast<uint32_t>(bank.size()), bank.data()};
    sa_audio_command setup[] = {midi(0, 0xb3, 32, 0), midi(0, 0xc3, 0, 0),
                                midi(0, 0xb3, 7, 127), midi(0, 0xb3, 10, 64),
                                midi(0, 0x93, 60, 127)};
    int16_t first = 0;

    if (sound.load(place) != SA_OK || sound.submit(setup, 5) != SA_OK) {
        return 4;
    }
    memset(bank.data(), 0, bank.size());
    /* at its own note it plays its 400 samples, at full level */
    uint32_t frames = heard(sound, first);

    if (frames != 400 || first != 0x4000) {
        fprintf(stderr, "sound: %u frames, first %d\n", frames, first);
        return 5;
    }
    /* an octave up, it plays half as long. a key past its split plays
     * nothing */
    sa_audio_command up = midi(0, 0x93, 72, 127);
    sa_audio_command past = midi(0, 0x93, 73, 127);

    sound.submit(&up, 1);
    if (heard(sound, first) != 200) {
        return 6;
    }
    sound.submit(&past, 1);
    if (heard(sound, first) != 0 || sound.stats().unplayed != 1) {
        return 7;
    }
    /* a note off ends it at its release's rate, before its sample ends */
    sa_audio_command on = midi(0, 0x93, 60, 127), off = midi(0, 0x83, 60, 0);
    int16_t frame[2];

    sound.submit(&on, 1);
    for (int i = 0; i < 10; i++) {
        sound.render(frame, 1);
    }
    sound.submit(&off, 1);
    frames = heard(sound, first);
    if (frames == 0 || frames >= 390) {
        return 8;
    }

    /* ADPCM: the codes of +7 take the step to its most, 0x6000. a code of
     * -7's change is then held at 0x7fff. so the sample falls to 0 and not
     * past it. the next code, 0, raises it by an eighth of the step */
    sa_audio_command adpcm[] = {midi(0, 0xc3, 1, 0), midi(0, 0x93, 60, 127)};
    int16_t codes[2 * 22];

    sound.submit(adpcm, 2);
    sound.render(codes, 22);
    if (codes[2 * 19] != 32767 || codes[2 * 20] != 0 ||
        codes[2 * 21] != 0x6000 / 8) {
        fprintf(stderr, "sound: ADPCM gives %d %d %d\n", codes[2 * 19],
                codes[2 * 20], codes[2 * 21]);
        return 9;
    }
    /* ADPCM interpolates between two samples as PCM does. its first two are
     * 238 and 808. at key 54, a half octave down, the second frame stands
     * 0.707 of the way between them */
    sa_audio_command lower = midi(0, 0x93, 54, 127);

    sound.submit(&lower, 1);
    sound.render(codes, 22);
    if (codes[0] != 238 || codes[2] < 640 || codes[2] > 642) {
        fprintf(stderr, "sound: ADPCM at key 54 gives %d %d\n", codes[0],
                codes[2]);
        return 22;
    }

    /* a sequence on port 2, at 44.1 frames a tick. its first note starts at
     * tick 1 and falls silent once its sample ends. its second starts at
     * tick 101 as the first's gate ends, and that end doesn't release it */
    std::vector<uint8_t> seq = sequences();
    sa_audio_bank seq_place = {SA_BANK_SEQUENCES, 0, BANK + 0x1000,
                               static_cast<uint32_t>(seq.size()), seq.data()};
    sa_audio_command play{};
    std::vector<int16_t> heard_seq(2 * 4700);

    play.kind = SA_AUDIO_SEQUENCE;
    play.key.id = 2;
    if (sound.load(seq_place) != SA_OK || sound.submit(&play, 1) != SA_OK) {
        return 10;
    }
    memset(seq.data(), 0, seq.size());
    sound.render(heard_seq.data(), 4700);
    if (heard_seq[2 * 30] != 0 || heard_seq[2 * 100] == 0 ||
        heard_seq[2 * 3000] != 0 || heard_seq[2 * 4440] != 0 ||
        heard_seq[2 * 4470] == 0 || heard_seq[2 * 4650] == 0) {
        return 11;
    }

    /* the DSP: program 2's note sends to input 0 alone. output 0 plays it
     * left as it comes, and output 1 plays it right 100 samples late */
    std::vector<uint8_t> fpb = fx_programs(), fob = fx_outputs();
    sa_audio_bank fx_places[] = {
        {SA_BANK_OTHER, 0, BANK + 0x2000, static_cast<uint32_t>(fpb.size()),
         fpb.data()},
        {SA_BANK_OTHER, 0, BANK + 0x3000, static_cast<uint32_t>(fob.size()),
         fob.data()}};
    sa_audio_command fx{};
    sa_audio_command sent[] = {midi(0, 0xc3, 2, 0), midi(0, 0x93, 60, 127)};
    std::vector<int16_t> wet(2 * 520);

    fx.kind = SA_AUDIO_FX_PROGRAM;
    sound.render(wet.data(), 400); /* the sequence's last note ends */
    if (sound.load(fx_places[0]) != SA_OK ||
        sound.load(fx_places[1]) != SA_OK || sound.submit(&fx, 1) != SA_OK ||
        sound.submit(sent, 2) != SA_OK) {
        return 15;
    }
    sound.render(wet.data(), 520);
    if (wet[0] < 16370 || wet[1] != 0 || wet[2 * 99 + 1] != 0 ||
        wet[2 * 100 + 1] < 16370 || wet[2 * 100] != wet[0] ||
        wet[2 * 400] != 0 || wet[2 * 499 + 1] != wet[2 * 100 + 1] ||
        wet[2 * 500 + 1] != 0) {
        fprintf(stderr, "sound: the DSP gives %d %d, %d %d, %d %d\n", wet[0],
                wet[1], wet[2 * 99 + 1], wet[2 * 100 + 1], wet[2 * 400],
                wet[2 * 500 + 1]);
        return 16;
    }
    /* qsound: program 5's note sends to input 1, which coefficient 84
     * gives output 2. a place without 0x80 moves nothing. place 4 with it
     * sets coefficient 84, channel 0's first, from the driver's table */
    std::vector<uint8_t> drv = driver();
    sa_audio_bank drv_place = {SA_BANK_OTHER, 0, 0,
                               static_cast<uint32_t>(drv.size()), drv.data()};
    sa_audio_command placed[] = {midi(0, 0xc3, 5, 0), midi(0, 0x93, 60, 127)};
    sa_audio_command where{};
    int16_t q[2 * 50];

    where.kind = SA_AUDIO_QSOUND;
    where.value = 0x04;
    if (sound.load(drv_place) != SA_OK || sound.submit(placed, 2) != SA_OK ||
        sound.submit(&where, 1) != SA_OK) {
        return 19;
    }
    sound.render(q, 50);
    int16_t unmarked = q[2 * 40];

    where.value = 0x84;
    sound.submit(&where, 1);
    sound.render(q, 50);
    if (unmarked != 0 || q[2 * 40] < 16370 || q[2 * 40 + 1] != 0) {
        fprintf(stderr, "sound: qsound gives %d, then %d %d\n", unmarked,
                q[2 * 40], q[2 * 40 + 1]);
        return 20;
    }
    sound.render(wet.data(), 400);
    /* the bank's table of levels: volume 106 takes 9 dB off a note at half
     * full scale */
    sa_audio_command quieter[] = {midi(0, 0xc3, 0, 0), midi(0, 0xb3, 7, 106),
                                  midi(0, 0x93, 60, 127)};
    sa_audio_command louder = midi(0, 0xb3, 7, 127);

    sound.submit(quieter, 3);
    sound.render(q, 1);
    sound.submit(&louder, 1);
    sound.render(wet.data(), 450);
    if (q[0] < 5810 || q[0] > 5822) {
        fprintf(stderr, "sound: volume 106 gives %d\n", q[0]);
        return 21;
    }
    /* the LFO: 24 dB off for the first half of its period, then full */
    sa_audio_command swung[] = {midi(0, 0xc3, 3, 0), midi(0, 0x93, 60, 127)};
    int16_t lfo[2 * 256];

    sound.submit(swung, 2);
    sound.render(lfo, 256);
    if (lfo[2 * 200] != 0x4000 || lfo[2 * 10] < 0x4000 / 17 ||
        lfo[2 * 10] > 0x4000 / 15) {
        fprintf(stderr, "sound: the LFO gives %d %d\n", lfo[2 * 10],
                lfo[2 * 200]);
        return 17;
    }
    sound.render(lfo, 150);
    /* the filter at 0x1600: a constant sample rises to its level over some
     * frames. unfiltered, it plays at its level at once */
    sa_audio_command closed[] = {midi(0, 0xc3, 4, 0), midi(0, 0x93, 60, 127)};
    int16_t low[2 * 300];

    sound.submit(closed, 2);
    sound.render(low, 300);
    if (low[0] <= 0 || low[0] > 0x4000 / 8 || low[2 * 299] < 0x4000 * 9 / 10) {
        fprintf(stderr, "sound: the filter gives %d %d\n", low[0],
                low[2 * 299]);
        return 18;
    }

    /* a filter held at 0x1ff8 passes a tone at half the rate within a tenth
     * of its level */
    sa_audio_command bright[] = {midi(0, 0xc3, 6, 0), midi(0, 0x93, 60, 127)};
    int16_t tone[2 * 100];
    int loudest = 0;

    sound.render(low, 300);
    sound.submit(bright, 2);
    sound.render(tone, 100);
    for (int i = 50; i < 100; i++) {
        loudest = abs(tone[2 * i]) > loudest ? abs(tone[2 * i]) : loudest;
    }
    if (loudest < 0x4000 * 9 / 10 || loudest > 0x4000) {
        fprintf(stderr, "sound: open filter leaves %d of half the rate\n",
                loudest);
        return 23;
    }

    /* mono: a note panned hard left plays at its level on both sides. a
     * stereo stream plays the sum of its channels on both */
    sa_audio_command space{};
    sa_audio_command panned[] = {midi(0, 0xc3, 0, 0), midi(0, 0xb3, 10, 0),
                                 midi(0, 0x93, 60, 127)};
    sa_audio_command centred = midi(0, 0xb3, 10, 64);
    int16_t both[2 * 50];

    file = adx();
    space.kind = SA_AUDIO_SPACE;
    space.value = 1;
    sound.render(low, 300);
    sound.submit(&space, 1);
    sound.submit(panned, 3);
    sound.render(both, 50);
    if (both[0] != 0x4000 || both[1] != 0x4000) {
        fprintf(stderr, "sound: mono plays a note %d %d\n", both[0], both[1]);
        return 26;
    }
    sound.submit(&centred, 1);
    sound.render(wet.data(), 400);
    sound.stream(0, file.data(), static_cast<uint32_t>(file.size()));
    sound.render(out.data(), 32 * PAIRS);
    for (uint32_t i = 0; i < 32 * PAIRS; i++) {
        int sum = left[i] + right[i];

        sum = sum > 32767 ? 32767 : sum < -32768 ? -32768 : sum;
        if (out[2 * i] != out[2 * i + 1] || abs(out[2 * i] - sum) > 1) {
            fprintf(stderr, "sound: mono plays frame %u %d %d, not %d\n", i,
                    out[2 * i], out[2 * i + 1], sum);
            return 27;
        }
    }
    /* the output returns to stereo, and the render empties the queue for the
     * next check */
    space.value = 0;
    sound.submit(&space, 1);
    sound.render(frame, 1);

    /* the queue holds SA_PC_SOUND_QUEUE commands and refuses the next. a
     * render empties it */
    sa_audio_command nothing{};

    for (uint32_t i = 0; i < SA_PC_SOUND_QUEUE; i++) {
        if (sound.submit(&nothing, 1) != SA_OK) {
            return 12;
        }
    }
    if (sound.submit(&nothing, 1) != SA_ERR_FULL ||
        sound.stats().refused != 1) {
        return 13;
    }
    sound.render(frame, 1);
    if (sound.submit(&nothing, 1) != SA_OK) {
        return 14;
    }
    return 0;
}

/* place bytes at address as bank number of kind */
sa_result place(pc::Sound &sound, uint32_t kind, uint32_t number,
                uint32_t address, const std::vector<uint8_t> &bytes)
{
    sa_audio_bank bank = {kind, number, address,
                          static_cast<uint32_t>(bytes.size()), bytes.data()};

    return sound.load(bank);
}

/* call it after check, which leaves bank 0 placed and port 0's channel 3 on
 * program 0. the DSP runs the fx bank's program */
int faults(pc::Sound &sound)
{
    /* load refuses a program bank whole where its mark is gone or a table
     * passes its bytes. the tables are the programs' and levels', program 0,
     * its layers 0 and 3 and layer 0's splits. a table passes by a few bytes,
     * or by a sum that wraps 32 bits. it also passes to the copy of the bank
     * that the data holds past the bank's size. a count's bytes pass the bank
     * or wrap 32 bits */
    const std::vector<uint8_t> good = programs();
    const uint32_t size = static_cast<uint32_t>(good.size());
    const struct {
        uint32_t at, value;
    } past[] = {
        {0x58, 0x05555556},   {0x14, 0x40000000},   {0x58, 0x40},
        {0x14, 0x300},        {0x10, size - 24},    {0x18, size - 127},
        {0x5e0, size - 15},   {0x40, size - 15},    {0x4c, size - 15},
        {0x5c, size - 0x2f},  {0x10, size + 0x5e0}, {0x18, size + 0x780},
        {0x5e0, size + 0x40}, {0x40, size + 0x58},  {0x5c, size + 0x68},
        {0x10, 0xfffffff0},   {0x18, 0xffffff80},   {0x5e0, 0xfffffff0},
        {0x40, 0xfffffff0},   {0x5c, 0xffffffd0},   {0, 0},
    };
    for (const auto &p : past) {
        std::vector<uint8_t> twice = good;

        twice.insert(twice.end(), good.begin(), good.end());
        put32(twice, p.at, p.value);
        sa_audio_bank bank = {SA_BANK_PROGRAMS, 0, BANK, size, twice.data()};
        if (sound.load(bank) != SA_ERR_INVALID) {
            return 30;
        }
    }
    /* a bank's header takes 0x30 bytes. a bank whose one program's offset
     * is 0, with no table of levels, is whole at 0x30 bytes. a byte shorter,
     * it is refused, though its programs' table lies within it */
    const uint32_t there = BANK + 0x8000;
    std::vector<uint8_t> ends = good;

    put32(ends, 0x10, 0x20);
    put32(ends, 0x14, 1);
    put32(ends, 0x18, 0);
    ends.resize(0x30);
    if (place(sound, SA_BANK_PROGRAMS, 1, there, ends) != SA_OK) {
        return 31;
    }
    ends.resize(0x2f);
    if (place(sound, SA_BANK_PROGRAMS, 1, there, ends) != SA_ERR_INVALID) {
        return 43;
    }
    /* a bank that ends with its last split is whole, and a byte shorter
     * isn't */
    ends = good;
    ends.resize(0xb88);
    if (place(sound, SA_BANK_PROGRAMS, 1, there, ends) != SA_OK) {
        return 44;
    }
    ends.resize(0xb87);
    if (place(sound, SA_BANK_PROGRAMS, 1, there, ends) != SA_ERR_INVALID) {
        return 45;
    }
    /* a refused bank leaves the one placed before it, which plays its note
     * as it did */
    sa_audio_command note = midi(0, 0x93, 60, 127);
    uint32_t unplayed = sound.stats().unplayed;
    int16_t first = 0;

    sound.submit(&note, 1);
    if (heard(sound, first) != 400 || first != 0x4000 ||
        sound.stats().unplayed != unplayed) {
        return 32;
    }
    /* bytes placed later may lie over a bank's tables. a note reads each
     * table within its bank, and goes unplayed where one has moved past
     * it. the programs' table, program 0, layer 0 and its splits are moved.
     * each moves to the bank's copy 0x1000 on, then by a sum that wraps 32
     * bits */
    const struct {
        uint32_t at, value;
    } moved[] = {
        {0x10, 0x1000 + 0x5e0}, {0x5e0, 0x1000 + 0x40}, {0x40, 0x1000 + 0x58},
        {0x5c, 0x1000 + 0x68},  {0x10, 0xfffffffc},     {0x5e0, 0xfffffff0},
        {0x40, 0xfffffff0},     {0x5c, 0xffffffd0},
    };
    sa_audio_command other[] = {midi(1, 0xb0, 32, 1), midi(1, 0xb0, 7, 127),
                                midi(1, 0x90, 60, 127)};
    std::vector<uint8_t> word(4);

    if (place(sound, SA_BANK_OTHER, 0, there + 0x1000, good) != SA_OK) {
        return 33;
    }
    for (const auto &m : moved) {
        put32(word, 0, m.value);
        if (place(sound, SA_BANK_PROGRAMS, 1, there, good) != SA_OK ||
            place(sound, SA_BANK_OTHER, 0, there + m.at, word) != SA_OK) {
            return 46;
        }
        unplayed = sound.stats().unplayed;
        sound.submit(other, 3);
        if (heard(sound, first) != 0 ||
            sound.stats().unplayed != unplayed + 1) {
            return 34;
        }
    }
    /* a count of splits past the bank stops at the bank's end. here the
     * splits start at the bank's last 0x30 bytes, which don't hold key 60.
     * the next split, just past the bank, holds it and goes unread */
    const std::vector<uint8_t> split(good.begin() + 0x68, good.begin() + 0x98);
    std::vector<uint8_t> splits(8);

    put32(splits, 0, 0x05555556);
    put32(splits, 4, size - 0x30);
    if (place(sound, SA_BANK_PROGRAMS, 1, there, good) != SA_OK ||
        place(sound, SA_BANK_OTHER, 0, there + size, split) != SA_OK ||
        place(sound, SA_BANK_OTHER, 0, there + 0x58, splits) != SA_OK) {
        return 47;
    }
    unplayed = sound.stats().unplayed;
    sound.submit(other, 3);
    if (heard(sound, first) != 0 || sound.stats().unplayed != unplayed + 1) {
        return 48;
    }
    /* a table of levels moved past its bank adds nothing. the note plays in
     * full at volume 127. the table moves to the bank's copy and before
     * the bank, where its levels would read as 0 */
    for (uint32_t levels : {0x1000U + 0x420, 0xffffff80U}) {
        put32(word, 0, levels);
        if (place(sound, SA_BANK_PROGRAMS, 1, there, good) != SA_OK ||
            place(sound, SA_BANK_OTHER, 0, there + 0x18, word) != SA_OK) {
            return 49;
        }
        sound.submit(other, 3);
        if (heard(sound, first) != 400 || first != 0x4000) {
            return 40;
        }
    }
    /* a level of 128 in the table counts as 127 and adds nothing. its rise
     * would take the note past its split's level */
    std::vector<uint8_t> loud = good;

    loud[0x780 + 127] = 128;
    if (place(sound, SA_BANK_PROGRAMS, 1, there, loud) != SA_OK) {
        return 50;
    }
    sound.submit(other, 3);
    if (heard(sound, first) != 400 || first != 0x4000) {
        return 41;
    }
    /* a voice whose step carries it past every sample sound memory can
     * hold ends there. this split of 8 bits loops over its first sample. its
     * own note is 0, and its bend's range 245. at key 127 bent to the top, it
     * steps 0x7fc7711c samples a frame. two frames on, its start plus its
     * index would wrap 32 bits to the bank's bytes at 0x280. those bytes
     * aren't 0, yet nothing sounds */
    std::vector<uint8_t> far = good;
    sa_audio_command bent[] = {midi(1, 0xe0, 0x7f, 0x7f),
                               midi(1, 0x90, 127, 127)};
    sa_audio_command unbent = midi(1, 0xe0, 0, 0x40);
    int16_t frame[2];

    put16(far, 0x68 + 0, 1 << 9 | 1 << 7 | 0x71); /* loops, 8 bits, SA */
    put16(far, 0x68 + 2, 0x2049); /* SA, 0x280 past the wrap */
    put16(far, 0x68 + 6, 1); /* LEA */
    far[0x68 + 36] = 0;
    far[0x68 + 37] = 127;
    far[0x68 + 38] = 0;
    far[0x64] = 245;
    memset(far.data() + 0x200, 0x40, 0x100);
    if (place(sound, SA_BANK_PROGRAMS, 1, there, far) != SA_OK ||
        sound.submit(bent, 2) != SA_OK) {
        return 51;
    }
    for (int i = 0; i < 3; i++) {
        sound.render(frame, 1);
        if (frame[0] != 0) {
            return 42;
        }
    }
    sound.submit(&unbent, 1);
    /* and the bank placed whole again plays */
    if (place(sound, SA_BANK_PROGRAMS, 1, there, good) != SA_OK) {
        return 52;
    }
    unplayed = sound.stats().unplayed;
    sound.submit(other, 3);
    if (heard(sound, first) != 400 || first != 0x4000 ||
        sound.stats().unplayed != unplayed) {
        return 35;
    }
    /* a sequence whose offset passes its bank goes unplayed. the offset
     * passes to an SMSD past the bank, by a byte, or by a sum that wraps */
    const std::vector<uint8_t> whole = sequences();
    const std::vector<uint8_t> smsd(whole.begin() + 0x20, whole.end());

    if (place(sound, SA_BANK_OTHER, 0, BANK + 0x1100 + 0x80, smsd) != SA_OK) {
        return 36;
    }
    for (uint32_t offset : {0x80U, 0x60U - 11, 0xfffffff8U}) {
        std::vector<uint8_t> seq = whole;
        sa_audio_command play{};

        put32(seq, 16, offset);
        play.kind = SA_AUDIO_SEQUENCE;
        play.key.id = 2;
        play.sound = 1 << 8;
        unplayed = sound.stats().unplayed;
        if (place(sound, SA_BANK_SEQUENCES, 1, BANK + 0x1100, seq) != SA_OK ||
            sound.submit(&play, 1) != SA_OK) {
            return 53;
        }
        sound.render(frame, 1);
        if (frame[0] != 0 || sound.stats().unplayed != unplayed + 1) {
            return 37;
        }
    }
    /* an fx program, its registers' image or an output set may lie past its
     * bank. the DSP then keeps the program and outputs it had, and program
     * 2's note sounds as it did. the program past the bank names an image 4
     * bytes past the bank's own. the image past it would be the outputs bank
     * and what follows. the set past its bank would read levels of 0. each
     * also passes by a sum that wraps */
    const struct {
        bool outputs;
        uint32_t at, value;
    } lost[] = {
        {false, 0x10, 0xd00},
        {false, 0x14 + 0x24, 0x1000},
        {true, 0x10, 0x40},
        {false, 0x10, 0xffffffe0},
        {false, 0x14 + 0x24, 0xfffff800},
        {true, 0x10, 0xfffffffe},
    };
    sa_audio_command sent[] = {midi(0, 0xc3, 2, 0), midi(0, 0x93, 60, 127)};
    sa_audio_command fx{};
    std::vector<int16_t> wet(2 * 520);
    std::vector<uint8_t> entry(0x28, 0);

    put32(entry, 0x24, 0x44);
    if (place(sound, SA_BANK_OTHER, 0, BANK + 0x2000 + 0xd00, entry) != SA_OK) {
        return 38;
    }
    fx.kind = SA_AUDIO_FX_PROGRAM;
    for (const auto &l : lost) {
        std::vector<uint8_t> fpb = fx_programs(), fob = fx_outputs();

        put32(l.outputs ? fob : fpb, l.at, l.value);
        if (place(sound, SA_BANK_OTHER, 0, BANK + 0x2000, fpb) != SA_OK ||
            place(sound, SA_BANK_OTHER, 0, BANK + 0x3000, fob) != SA_OK ||
            sound.submit(&fx, 1) != SA_OK || sound.submit(sent, 2) != SA_OK) {
            return 54;
        }
        sound.render(wet.data(), 520);
        if (wet[0] < 16370 || wet[1] != 0 || wet[2 * 99 + 1] != 0 ||
            wet[2 * 100 + 1] < 16370 || wet[2 * 500 + 1] != 0) {
            fprintf(stderr, "sound: a lost fx table leaves %d %d, %d %d\n",
                    wet[0], wet[1], wet[2 * 99 + 1], wet[2 * 100 + 1]);
            return 39;
        }
    }
    return 0;
}

/* after faults: nothing sounds, and the DSP runs on */
int quietest(pc::Sound &sound)
{
    /* a stream's volume falls no further than the port's -127. so -998
     * tenths of a dB plays as the least, -999, does. two streams of one file
     * there each stay under one step of the output. their sum reaches one
     * step where the file is loudest */
    std::vector<uint8_t> file = adx();
    std::vector<int16_t> least(2 * 32 * PAIRS), above(2 * 32 * PAIRS);
    sa_audio_command volume[2]{}, stop[2]{};
    int loudest = 0;

    for (uint32_t i = 0; i < 2; i++) {
        volume[i].kind = SA_AUDIO_STREAM_VOLUME;
        volume[i].key.id = i;
        stop[i].kind = SA_AUDIO_STREAM_STOP;
        stop[i].key.id = i;
    }
    for (int32_t asked : {-999, -998}) {
        std::vector<int16_t> &out = asked == -999 ? least : above;

        volume[0].value = volume[1].value = asked;
        for (uint32_t i = 0; i < 2; i++) {
            if (sound.submit(&volume[i], 1) != SA_OK ||
                sound.stream(i, file.data(),
                             static_cast<uint32_t>(file.size())) != SA_OK) {
                return 55;
            }
        }
        sound.render(out.data(), 32 * PAIRS);
        sound.submit(stop, 2);
    }
    for (int16_t sample : least) {
        loudest = abs(sample) > loudest ? abs(sample) : loudest;
    }
    if (least != above || loudest != 1) {
        fprintf(stderr, "sound: the quietest streams reach %d\n", loudest);
        return 56;
    }
    volume[0].value = volume[1].value = 0;
    sound.submit(volume, 2);

    /* a bank with no table of levels adds 46 steps an octave below volume
     * 127. below volume 3 they pass 255, the most an addition takes. so
     * volume 1 plays as volume 0, the least, does. 32 notes of program 0
     * there each stay under one step of the output. their sum passes one */
    std::vector<uint8_t> plain = programs();
    sa_audio_command set[] = {midi(1, 0xb0, 32, 1), midi(1, 0xc0, 0, 0),
                              midi(1, 0xb0, 7, 1)};
    sa_audio_command note = midi(1, 0x90, 60, 127);
    sa_audio_command none = midi(1, 0xb0, 7, 0), full = midi(1, 0xb0, 7, 127);
    int16_t at_one[2], at_none[2], first = 0;

    put32(plain, 0x1c, 0);
    if (place(sound, SA_BANK_PROGRAMS, 1, BANK + 0x8000, plain) != SA_OK ||
        sound.submit(set, 3) != SA_OK) {
        return 57;
    }
    for (int i = 0; i < 32; i++) {
        sound.submit(&note, 1);
    }
    sound.render(at_one, 1);
    sound.submit(&none, 1);
    sound.render(at_none, 1);
    if (at_one[0] <= 1 || at_one[0] != at_none[0]) {
        fprintf(stderr, "sound: volume 1 gives %d, and 0 %d\n", at_one[0],
                at_none[0]);
        return 58;
    }
    sound.submit(&full, 1);
    (void)heard(sound, first);
    return 0;
}

/* commands for port 0's channel 0, and the notes a sound that carries them
 * out in order leaves unplayed. a command takes program 0 or 7, or plays key
 * 60. programs()'s program 0 plays the key, and its bank holds no program 7.
 * so the count tells a command lost, carried out twice or out of its place.
 * a batch of two or more ends on program 7 and a note, so the count tells
 * its last command too */
struct Script {
    uint32_t seed = 1;
    uint32_t program = 0;
    uint32_t unplayed = 0;

    std::vector<sa_audio_command> batch(uint32_t count)
    {
        std::vector<sa_audio_command> commands;

        for (uint32_t left = count; left != 0; left--) {
            seed = seed * 1103515245 + 12345;
            if (left == 2 || (left != 1 && (seed >> 16) % 3 == 0)) {
                program = left == 2 || (seed >> 20 & 1) != 0 ? 7 : 0;
                commands.push_back(midi(0, 0xc0, program, 0));
            } else {
                unplayed += program != 0;
                commands.push_back(midi(0, 0x90, 60, 127));
            }
        }
        return commands;
    }
};

/* a batch of commands is queued whole or not at all, on a sound of its own.
 * a batch the queue has no room for leaves the queue as it found it */
int batches()
{
    static pc::Sound sound;
    const uint32_t room = 3;
    Script script;
    std::vector<sa_audio_command> batch;
    int16_t frame[2];
    auto submit = [&](uint32_t count) {
        batch = script.batch(count);
        return sound.submit(batch.data(), count);
    };

    if (!sound.create()) {
        return 99;
    }
    if (place(sound, SA_BANK_PROGRAMS, 0, BANK, programs()) != SA_OK) {
        return 70;
    }
    sound.render(frame, 1);
    /* no commands are taken, and one is. a batch then leaves room for 3 */
    if (sound.submit(nullptr, 0) != SA_OK || submit(1) != SA_OK ||
        submit(SA_PC_SOUND_QUEUE - 1 - room) != SA_OK ||
        sound.stats().most_queued != SA_PC_SOUND_QUEUE - room) {
        return 71;
    }
    /* one command more than the room is refused, and none of it is queued */
    const Script before = script;

    if (submit(room + 1) != SA_ERR_FULL ||
        sound.stats().most_queued != SA_PC_SOUND_QUEUE - room ||
        sound.stats().refused != room + 1) {
        return 72;
    }
    script = before;
    /* so the room takes a batch of its size. the full queue then takes a
     * batch of none, and refuses one command */
    if (submit(room) != SA_OK || sound.submit(nullptr, 0) != SA_OK ||
        sound.submit(batch.data(), 1) != SA_ERR_FULL) {
        return 73;
    }
    /* the render carries out the batches taken, each command once and in
     * its order, and none of the batch refused */
    sound.render(frame, 1);
    if (sound.stats().unplayed != script.unplayed) {
        return 74;
    }
    /* a batch leaves the queue's next place 100 before its end. so a batch
     * of 200 runs past the end and on from the start */
    if (submit(SA_PC_SOUND_QUEUE - 101) != SA_OK) {
        return 75;
    }
    sound.render(frame, 1);
    if (submit(200) != SA_OK) {
        return 76;
    }
    sound.render(frame, 1);
    if (sound.stats().unplayed != script.unplayed) {
        return 77;
    }
    sound.destroy();
    return 0;
}

} // namespace

int main()
{
    static pc::Sound sound;

    if (!sound.create()) {
        return 99;
    }
    int failed = check(sound);

    failed = failed ? failed : loops(sound);
    failed = failed ? failed : faults(sound);
    failed = failed ? failed : quietest(sound);
    failed = failed ? failed : batches();

    sound.destroy();
    if (failed) {
        fprintf(stderr, "sound: check %d fails\n", failed);
    }
    return failed;
}
