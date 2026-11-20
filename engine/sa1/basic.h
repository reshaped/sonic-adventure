/* Ninja's basic models, as the SH-4 lays them out, turned into
 * SA_FORMAT_MESH's triangles */
#ifndef ENGINE_SA1_BASIC_H
#define ENGINE_SA1_BASIC_H

#include <stdint.h>

#include <string>
#include <vector>

namespace pc {

/* a meshset's triangles, three corners each, as points' indexes. a u or v
 * is the file's signed 16-bit value divided by 255. a strip's odd triangles
 * swap their first two corners, or a reversed strip's even ones. every
 * triangle then faces the way its strip does */
struct Meshset {
    uint32_t material = 0;
    std::vector<uint16_t> corners;
    std::vector<float> uv; /* 2 a corner, or none */
};

struct Model {
    std::vector<float> points; /* x, y, z */
    std::vector<float> normals; /* x, y, z, or none */
    std::vector<Meshset> meshsets;
    uint32_t materials = 0;
};

/* read into model the NJS_MODEL at offset in file, whose bytes start at
 * the address base. it returns false and sets why where a read, its points
 * or its meshsets leave file. a model of over 2048 points fails too, as
 * does a corner naming a point it lacks. a meshset of no polygons may point
 * anywhere */
bool ninja_model(const std::vector<uint8_t> &file, uint32_t base,
                 uint32_t offset, Model &model, std::string &why);

/* model in SA_FORMAT_MESH's form */
void mesh_bytes(const Model &model, std::vector<uint8_t> &out);

} // namespace pc

#endif
