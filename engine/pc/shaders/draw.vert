#version 450
// a corner of a draw, in world space, or under BATCH_SCREEN at the screen's
// pixels with its view z. the screen is 640 by 480, and a point shows at
// center + focal * (x, y) / z in view space, z ahead. depth is 1/z, as the
// PowerVR compares it, so z = 1 is the near plane
layout(set = 0, binding = 0) uniform Picture {
    vec4 view[3]; // world to view, by rows
    vec4 project; // focal x and y, center x and y
    vec4 fog_color;
    vec4 fog; // density, and the near clip
    vec4 table[64]; // the fog table's entries, two a vector, each high and low
} picture;
layout(push_constant) uniform Batch {
    uint flags;
    uint blend;
} batch;
layout(location = 0) in vec3 at;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 base;
layout(location = 3) in vec4 offset;
layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_base;
layout(location = 2) out vec4 out_offset;
// the triangle's place in the picture, which settles a tie in the blend list
layout(location = 3) flat out uint out_triangle;
out gl_PerVertex {
    vec4 gl_Position;
    float gl_ClipDistance[1];
};

const uint BATCH_SCREEN = 16u;
const uint BATCH_NEAR = 256u; // clip at the near clip, not at z = 1

void main()
{
    vec3 view = at;
    vec2 screen;

    if ((batch.flags & BATCH_SCREEN) == 0u) {
        vec4 p = vec4(at, 1.0);
        view = vec3(dot(picture.view[0], p), dot(picture.view[1], p), dot(picture.view[2], p));
        screen = picture.project.xy * view.xy + picture.project.zw * view.z;
    } else {
        screen = at.xy * view.z;
    }
    gl_Position = vec4(screen.x / 320.0 - view.z, screen.y / 240.0 - view.z, 1.0, view.z);
    gl_ClipDistance[0] = (batch.flags & BATCH_NEAR) != 0u ? view.z - picture.fog.y : 1.0;
    out_uv = uv;
    out_base = base;
    out_offset = offset;
    out_triangle = uint(gl_VertexIndex) / 3u;
}
