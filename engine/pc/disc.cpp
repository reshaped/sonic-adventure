#include "disc.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>

namespace pc {

float Bytes::f32(uint32_t at)
{
    uint32_t bits = u32(at);
    float value;

    memcpy(&value, &bits, sizeof value);
    return value;
}

/* control bits come low first, from a byte read when the one before runs out.
 * a set bit copies the next byte. a clear bit then a set one is a long copy by
 * a little-endian word. a word of 0 ends the stream. the word's top 13 bits
 * say how far back, and its low 3 bits give the length. where those 3 are 0,
 * the next byte gives it. two clear bits are a short copy. two more control
 * bits give its length, and the next byte gives how far back it starts */
bool prs_decode(const std::vector<uint8_t> &in, std::vector<uint8_t> &out)
{
    size_t at = 0;
    uint32_t control = 0;
    int bits = 0;
    auto bit = [&]() -> int {
        if (bits == 0) {
            if (at >= in.size()) {
                return -1;
            }
            control = in[at++];
            bits = 8;
        }
        int b = static_cast<int>(control & 1);
        control >>= 1;
        bits--;
        return b;
    };

    out.clear();
    for (;;) {
        int b = bit();
        int64_t distance;
        uint32_t length;

        if (b < 0) {
            return false;
        }
        if (b == 1) {
            if (at >= in.size()) {
                return false;
            }
            out.push_back(in[at++]);
            continue;
        }
        b = bit();
        if (b < 0) {
            return false;
        }
        if (b == 1) {
            if (at + 2 > in.size()) {
                return false;
            }
            uint32_t word = in[at] | in[at + 1] << 8;
            at += 2;
            if (word == 0) {
                return true;
            }
            distance = static_cast<int64_t>(word >> 3) - 0x2000;
            length = word & 7;
            if (length == 0) {
                if (at >= in.size()) {
                    return false;
                }
                length = in[at++] + 1U;
            } else {
                length += 2;
            }
        } else {
            int high = bit();
            int low = bit();
            if (high < 0 || low < 0 || at >= in.size()) {
                return false;
            }
            length = static_cast<uint32_t>(high << 1 | low) + 2;
            distance = static_cast<int64_t>(in[at++]) - 0x100;
        }
        if (static_cast<int64_t>(out.size()) + distance < 0) {
            return false;
        }
        size_t from = out.size() + distance;
        for (uint32_t i = 0; i < length; i++) {
            uint8_t c = out[from + i];

            out.push_back(c);
        }
    }
}

bool Disc::read(const std::string &path, std::vector<uint8_t> &bytes) const
{
    for (const std::string &root : roots_) {
        FILE *file = fopen((root + "/" + path).c_str(), "rb");

        if (file == nullptr) {
            continue;
        }
        fseek(file, 0, SEEK_END);
        long size = ftell(file);

        if (size < 0) {
            fclose(file);
            return false;
        }
        fseek(file, 0, SEEK_SET);
        bytes.resize(static_cast<size_t>(size));
        size_t got =
            bytes.empty() ? 0 : fread(bytes.data(), 1, bytes.size(), file);
        fclose(file);
        return got == bytes.size();
    }
    return false;
}

std::vector<std::string> Disc::list(const std::string &dir,
                                    const std::string &suffix) const
{
    std::vector<std::string> paths;

    for (const std::string &root : roots_) {
        DIR *d = opendir((root + "/" + dir).c_str());
        size_t first = paths.size();

        if (d == nullptr) {
            continue;
        }
        while (const dirent *entry = readdir(d)) {
            std::string name = entry->d_name;
            std::string path = dir + "/" + name;

            if (name.size() >= suffix.size() &&
                name.compare(name.size() - suffix.size(), suffix.size(),
                             suffix) == 0 &&
                std::find(paths.begin(), paths.end(), path) == paths.end()) {
                paths.push_back(path);
            }
        }
        closedir(d);
        /* readdir's order is the file system's, which differs by host */
        std::sort(paths.begin() + first, paths.end());
    }
    return paths;
}

} // namespace pc
