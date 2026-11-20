// shared by the ray tracing passes: the picture's uniforms, and the functions
// more than one pass uses. a pass includes this file, which is not compiled on
// its own

layout(set = 0, binding = 1) uniform Picture {
    vec4 view[3];
    vec4 project;
    vec4 fog_color;
    vec4 fog;
    vec4 table[64];
} picture;
layout(set = 0, binding = 2) uniform Rays {
    vec4 eye[3]; // view to world, 3 by 4, by rows: the last column is the eye
    vec4 to_sun; // the unit direction to the sun
    vec4 back[3]; // the background's color at the top left, top right and bottom left
    uvec4 counts; // the picture's number, a spare word, the rays a pixel, and 1 after a cut
    // the traced picture's size in pixels, then a pixel's size in the screen's
    // 640 by 480. it is the target's size, or a share of it, which the
    // antialiasing pass fills the target from
    vec4 size;
    vec4 before_view[3]; // the picture before's world to view, then its project
    vec4 before_project;
    vec4 clock; // the picture's time, then the time since the traced picture before, in seconds
    // x is LAMP_FLOOR, the least light of a lamp on a surface that counts, and y
    // LAMP_REACH, how far from its shape a lamp lights anything
    vec4 lamp;
    vec4 shown; // the target's size in pixels, then a pixel's size in the screen
    // where in its pixel this picture's camera rays cross it, from the
    // pixel's center, in pixels, then the same for the picture before
    vec4 jitter;
} rays;

// a 3 by 4 matrix, by rows, applied to a direction and to a point
vec3 rotate(vec4 m[3], vec3 v)
{
    return vec3(dot(m[0].xyz, v), dot(m[1].xyz, v), dot(m[2].xyz, v));
}

vec3 transform(vec4 m[3], vec3 p)
{
    vec4 q = vec4(p, 1.0);

    return vec3(dot(m[0], q), dot(m[1], q), dot(m[2], q));
}

// luminance, by Rec. 709's weights
float brightness(vec3 c)
{
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

// a unit vector's octahedral encoding, from -1 to 1 on each axis
vec2 folded(vec3 n)
{
    n /= abs(n.x) + abs(n.y) + abs(n.z);
    vec2 side = vec2(n.x >= 0.0 ? 1.0 : -1.0, n.y >= 0.0 ? 1.0 : -1.0);

    return n.z >= 0.0 ? n.xy : (1.0 - abs(n.yx)) * side;
}

// the unit vector of an octahedral encoding
vec3 unfolded(vec2 f)
{
    vec3 n = vec3(f, 1.0 - abs(f.x) - abs(f.y));
    float under = max(-n.z, 0.0);

    n.xy += vec2(n.x >= 0.0 ? -under : under, n.y >= 0.0 ? -under : under);
    return normalize(n);
}

// linear light as display values: tone mapped by the Khronos PBR Neutral curve,
// without its toe, then gamma-encoded at 2.2. light up to KEPT is unchanged,
// and brighter light is compressed below 1 and desaturated at the rate PALES
const float KEPT = 0.76;
const float PALES = 0.15;

vec3 displayed(vec3 light)
{
    vec3 c = max(light, 0.0);
    float peak = max(c.r, max(c.g, c.b));

    if (peak >= KEPT) {
        float room = 1.0 - KEPT;
        float pressed = 1.0 - room * room / (peak + room - KEPT);

        c *= pressed / peak;
        c = mix(c, vec3(pressed), 1.0 - 1.0 / (PALES * (peak - pressed) + 1.0));
    }
    return pow(c, vec3(1.0 / 2.2));
}

// the fog table's alpha at a depth of w = 1/z, as draw.frag's fog_alpha
// computes it
float fog_alpha(float w)
{
    float z = clamp(w * picture.fog.x, 1.0, 255.9999);
    float e = floor(log2(z));
    float m = z * 16.0 / exp2(e) - 16.0;
    int index = int(floor(m) + e * 16.0);
    vec4 pair = picture.table[index >> 1];
    vec2 entry = (index & 1) == 0 ? pair.xy : pair.zw;

    return mix(entry.x, entry.y, fract(m)) / 255.0;
}
