#include "png.h"

#include <stdio.h>

namespace pc {

namespace {

uint32_t crc32(const uint8_t *data, size_t size, uint32_t crc = 0)
{
    crc = ~crc;
    for (size_t i = 0; i < size; i++) {
        crc ^= data[i];
        for (int k = 0; k < 8; k++) {
            crc = crc & 1 ? (crc >> 1) ^ 0xedb88320U : crc >> 1;
        }
    }
    return ~crc;
}

void be32(std::vector<uint8_t> &out, uint32_t v)
{
    out.push_back(static_cast<uint8_t>(v >> 24));
    out.push_back(static_cast<uint8_t>(v >> 16));
    out.push_back(static_cast<uint8_t>(v >> 8));
    out.push_back(static_cast<uint8_t>(v));
}

void chunk(std::vector<uint8_t> &out, const char *tag,
           const std::vector<uint8_t> &data)
{
    be32(out, static_cast<uint32_t>(data.size()));
    size_t start = out.size();
    out.insert(out.end(), tag, tag + 4);
    out.insert(out.end(), data.begin(), data.end());
    be32(out, crc32(&out[start], out.size() - start));
}

} // namespace

bool png_write(const std::string &path, uint32_t width, uint32_t height,
               const std::vector<uint8_t> &rgba)
{
    std::vector<uint8_t> raw;
    std::vector<uint8_t> z;
    std::vector<uint8_t> header;
    std::vector<uint8_t> out = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    uint32_t a = 1;
    uint32_t b = 0;

    for (uint32_t y = 0; y < height; y++) {
        raw.push_back(0);
        raw.insert(raw.end(), rgba.begin() + y * width * 4,
                   rgba.begin() + (y + 1) * width * 4);
    }
    for (uint8_t v : raw) {
        a = (a + v) % 65521;
        b = (b + a) % 65521;
    }
    /* zlib's header, then stored blocks of at most 65535 bytes, then the
     * Adler-32 of what they hold */
    z.push_back(0x78);
    z.push_back(0x01);
    for (size_t at = 0; at < raw.size(); at += 65535) {
        size_t n = raw.size() - at < 65535 ? raw.size() - at : 65535;

        z.push_back(at + n == raw.size() ? 1 : 0);
        z.push_back(static_cast<uint8_t>(n));
        z.push_back(static_cast<uint8_t>(n >> 8));
        z.push_back(static_cast<uint8_t>(~n));
        z.push_back(static_cast<uint8_t>(~n >> 8));
        z.insert(z.end(), raw.begin() + at, raw.begin() + at + n);
    }
    be32(z, b << 16 | a);
    be32(header, width);
    be32(header, height);
    header.push_back(8); /* bits a channel */
    header.push_back(6); /* RGBA */
    header.push_back(0);
    header.push_back(0);
    header.push_back(0);
    chunk(out, "IHDR", header);
    chunk(out, "IDAT", z);
    chunk(out, "IEND", {});
    FILE *f = fopen(path.c_str(), "wb");
    if (f == nullptr) {
        return false;
    }
    bool ok = fwrite(out.data(), 1, out.size(), f) == out.size();
    return fclose(f) == 0 && ok;
}

} // namespace pc

int pc_png_write(const char *path, uint32_t width, uint32_t height,
                 const uint8_t *rgba)
{
    std::vector<uint8_t> rows(rgba, rgba + size_t{width} * height * 4);

    return pc::png_write(path, width, height, rows) ? 1 : 0;
}
