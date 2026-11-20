#version 450
// a traced picture's pixel, with the depth of the camera ray's hit as 1/z, so
// that the batches drawn after it are depth-tested against the traced surfaces.
// the depth is the traced picture's, which may be a share of the target's size
layout(set = 0, binding = 0, rgba8) uniform readonly image2D shown;
layout(set = 0, binding = 1, r32f) uniform readonly image2D depth;
layout(location = 0) out vec4 color;

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec2 share = vec2(imageSize(depth)) / vec2(imageSize(shown));
    float z = imageLoad(depth, min(ivec2(gl_FragCoord.xy * share), imageSize(depth) - 1)).r;

    color = imageLoad(shown, pixel);
    gl_FragDepth = z > 0.0 ? clamp(1.0 / z, 0.0, 1.0) : 0.0;
}
