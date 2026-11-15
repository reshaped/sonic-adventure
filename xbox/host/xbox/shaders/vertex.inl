// the Xbox host's vertex program, by hand in NV_vertex_program 1.1, which
// vp20compiler takes as Cg's output, so that every constant is the host's
// (Cg's own literals went to c[0], which the host didn't load).
//
// its vertices are the renderer's as they come (struct host_vertex, 28
// bytes): v[0] x, y and rhw, v[8] u and v, v[3] the base color, v[4] the
// offset color, its alpha the PowerVR's vertex fog. for each vertex
// (video.c's head):
// - the position: x and y in pixels, the depth of rhw (depth_of's two
//   pieces, linear in rhw), w = 1/rhw, rhw kept above 1e-7; x and y
//   then times c[78]'s, the display's pixels to the game's 640 by 480
//   (1 and 1, or 2 and 1.5 at 720p);
// - the texture coordinates scaled to the stored texture;
// - the base color's alpha forced to 1 where the polygon ignores it, and the
//   offset color where it has none;
// - the fog's texture coordinate, TEXCOORD1, for the fog texture on stage 1,
//   which gives a pixel its fog's share (video.c's fog): s over q is the
//   table's place for the pixel's own rhw, s = A + B * w over q = w, which
//   across a triangle is A times rhw, linear on the screen as the PowerVR's
//   is, plus B; or the vertex's fog on a ramp, s = A + V * the offset
//   color's alpha over q = 1.
//
// the constants, which video.c loads (at 96 + n):
//   c[0]  1, 0, 0.5, 16
//   c[1]  the depth's knee, its scale below, its scale above, the depth there
//   c[2]  the depth's top, 0, 1e-7, 255.9999
//   c[6]  the depth's bottom piece: the rhw it reaches, its scale, the depth
//         there (video.c's depth_next: the 2D layers far behind the 3D)
//   c[3]  the texture's u and v scales, 0, 1                     (a draw's)
//   c[4]  the fog's A, B and V, and 1 where the base color's alpha is
//         ignored                                                 (a draw's)
//   c[5]  the fog's q: its share of w, and its constant; 0; 1 where the
//         offset color counts                                     (a draw's)
//   c[72] to c[74]  a model's transform (a host_item's): the rows that give
//         a point's x and y on the screen, each times its depth z, and z.
//         for vertices already on the screen, the rows of x, y and 1
//   c[75] 1, 0 for a model's vertices, whose rhw is 1 / z, and 0, 1 for
//         those on the screen, whose third coordinate is rhw
// the position: a model's point through its transform, x and y over z and
// rhw 1 / z, or a vertex on the screen as it is
0x00000000, 0x00f5001b, 0x0836186c, 0x28a007f8,
0x00000000, 0x00f5201b, 0x0836186c, 0x24a007f8,
0x00000000, 0x00f5401b, 0x0836186c, 0x22a007f8,
0x00000000, 0x0400001b, 0x08361202, 0x90a107f8,
0x00000000, 0x0040001b, 0xa5ff486c, 0x2ca007f8,
0x00000000, 0x005560ff, 0xa400186c, 0x21a007f8,
0x00000000, 0x009560aa, 0x08aa1bfe, 0x91a007f8,
0x00000000, 0x014c40ff, 0xa554186c, 0x280007f8,
0x00000000, 0x0400001b, 0x08361000, 0x100407f8,
0x00000000, 0x00200055, 0x0436106c, 0x20701800,
0x00000000, 0x006cc000, 0x04361400, 0x384007f8,
0x00000000, 0x004c2000, 0x44aa186c, 0x281007f8,
0x00000000, 0x006cc000, 0x143612a8, 0x381007f8,
0x00000000, 0x004cc000, 0x04aa186c, 0x244007f8,
0x00000000, 0x016cc000, 0x0400186c, 0x224007f8,
0x00000000, 0x00600055, 0x44361400, 0x514007f8,
0x00000000, 0x008000aa, 0x45fe8800, 0x581007f8,
0x00000000, 0x006c2000, 0x04361400, 0x341007f8,
0x00000000, 0x008c2055, 0x15541bfc, 0x341007f8,
0x00000000, 0x016c2000, 0x0c00086c, 0x221007f8,
0x00000000, 0x00600055, 0x14361400, 0x511007f8,
0x00000000, 0x008000aa, 0x15fe2800, 0x581007f8,
0x00000000, 0x012c4000, 0x1400186c, 0x20702800,
0x00000000, 0x0055c01b, 0xa436186c, 0x2070c800,
0x00000000, 0x004c721b, 0x0836186c, 0x2070c848,
0x00000000, 0x002c601b, 0x0c36106c, 0x20703848,
0x00000000, 0x008c8055, 0x04aa1800, 0x382007f8,
0x00000000, 0x008c88ff, 0x09541800, 0x90708850,
0x00000000, 0x002c0055, 0x0c36106c, 0x20706850,
0x00000000, 0x008ca055, 0x04001954, 0x30701850,
0x00000000, 0x004ca81b, 0x09fe186c, 0x2070e820,
0x00000000, 0x002c0055, 0x0c36106c, 0x20701820,
0x00000000, 0x0020061b, 0x0836106c, 0x2fa007f8,
0x00000000, 0x014c86ff, 0x09fe186c, 0x21a007f8,
0x00000000, 0x0020001b, 0xa436106c, 0x2070f819,
