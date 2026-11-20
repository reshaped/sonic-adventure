#version 450
// a fragment the blend list keeps leaves it only once the depth test passes
layout(early_fragment_tests) in;
// a pixel as the PowerVR shades it: the texel and the corner's colors by the
// batch's shading, then the fog the batch's mode asks for
layout(set = 0, binding = 0) uniform Picture {
    vec4 view[3];
    vec4 project;
    vec4 fog_color;
    vec4 fog;
    vec4 table[64];
} picture;
// the blend list's fragments, each pixel's a list from its head, which the
// resolve sorts as the PowerVR does
layout(set = 0, binding = 1, r32ui) uniform coherent uimage2D heads;
struct Fragment {
    uint color; // RGBA8
    float depth; // 1/z
    uint next;
    uint order; // the blend's instructions, then the triangle's place
};
layout(set = 0, binding = 2, std430) buffer Fragments {
    uint count;
    uint capacity;
    uint lost; // fragments the list had no room for, which joined no pixel
    uint cut; // the resolve's: fragments a pixel held past those it composes
    Fragment fragments[];
} list;
layout(set = 1, binding = 0) uniform sampler2D texel;
layout(push_constant) uniform Batch {
    uint flags;
    uint blend; // the source's instruction, then the destination's, three bits each
} batch;
layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 base;
layout(location = 2) in vec4 offset;
layout(location = 3) flat in uint triangle;
layout(location = 0) out vec4 color;

const uint BATCH_TEXTURE = 1u;
const uint BATCH_DECAL = 2u;
const uint BATCH_OFFSET = 4u;
const uint BATCH_OPAQUE = 8u; // the texel's alpha counts as 1
const uint FOG_SHIFT = 5u; // two bits: table, vertex, none, table mode 2
const uint BATCH_COLLECT = 128u; // keep the fragment for the resolve

// the fog table's alpha for this pixel: 1/z times the density, held to
// 1 to 256, picks an entry by its exponent and the mantissa's top four bits,
// and the rest of the mantissa moves from the entry's high byte to its low
float fog_alpha()
{
    float z = clamp(gl_FragCoord.w * picture.fog.x, 1.0, 255.9999);
    float e = floor(log2(z));
    float m = z * 16.0 / exp2(e) - 16.0;
    int index = int(floor(m) + e * 16.0);
    vec4 pair = picture.table[index >> 1];
    vec2 entry = (index & 1) == 0 ? pair.xy : pair.zw;
    return mix(entry.x, entry.y, fract(m)) / 255.0;
}

void main()
{
    vec4 c = base;

    if ((batch.flags & BATCH_TEXTURE) != 0u) {
        vec4 t = texture(texel, uv);

        if ((batch.flags & BATCH_OPAQUE) != 0u) {
            t.a = 1.0;
        }
        c = (batch.flags & BATCH_DECAL) != 0u ? t : t * base;
        if ((batch.flags & BATCH_OFFSET) != 0u) {
            c.rgb = min(c.rgb + offset.rgb, vec3(1.0));
        }
    }
    uint mode = (batch.flags >> FOG_SHIFT) & 3u;
    if (mode == 0u) {
        c.rgb = mix(c.rgb, picture.fog_color.rgb, fog_alpha());
    } else if (mode == 1u) {
        c.rgb = mix(c.rgb, picture.fog_color.rgb, offset.a);
    } else if (mode == 3u) {
        c = vec4(picture.fog_color.rgb, fog_alpha());
    }
    color = c;
    if ((batch.flags & BATCH_COLLECT) != 0u) {
        uint at = atomicAdd(list.count, 1u);
        if (at >= list.capacity) {
            atomicAdd(list.lost, 1u);
            return;
        }
        uvec4 bytes = uvec4(clamp(c, 0.0, 1.0) * 255.0 + 0.5);
        list.fragments[at].color = bytes.r | bytes.g << 8 | bytes.b << 16 | bytes.a << 24;
        list.fragments[at].depth = gl_FragCoord.z;
        list.fragments[at].order = batch.blend | triangle << 6;
        list.fragments[at].next = imageAtomicExchange(heads, ivec2(gl_FragCoord.xy), at);
    }
}
