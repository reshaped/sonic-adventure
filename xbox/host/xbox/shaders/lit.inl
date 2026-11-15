// the Xbox host's vertex program for a model it lights (host.h's
// host_item, video.c's lit runs): shaders/vertex.vp, but that
// - v[2] is the vertex's normal in the model's space, its offset field read
//   as the NV2A's CMP, and its base color's RGB is the palette's: the
//   combiners (shaders/lit_NAME.ps) take the base color's RGB from stage 2's
//   texture and the offset color from stage 3's, the palette's two rows, at
//   the place TEXCOORD2 and TEXCOORD3 give, and the vertex's colors give
//   only the base color's alpha and the offset color's share;
// - the vertex's own fog is none, as its offset field holds the normal.
//
// the place is the normal's dot with the light, d, times 127.5 / 256 plus
// 127.5 / 256: the entry (int)(d * 127.5 + 127.5) of the drawer's 256 is
// its texel. the texture's filter blends between entries, which over a
// triangle gives what the PowerVR's blend of its vertices' entries gives,
// within an entry's step.
//
// its constants are vertex.vp's, and (at 96 + n):
//   c[76]  the light in the model's space, 0                     (a run's)
//   c[77]  127.5 / 256, 127.5 / 256, the base colors' row, the offset
//          colors' row
// the position: a model's point through its transform, x and y over z and
// rhw 1 / z, or a vertex on the screen as it is
// the normal's
// the entry (int)(d * 127.5 + 127.5) of 256 is its texel, the base colors'
// row and the offset colors'
// 1 where it counts
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
0x00000000, 0x008c8055, 0x04aa1800, 0x30708850,
0x00000000, 0x002c0055, 0x0c36106c, 0x20706850,
0x00000000, 0x008ca055, 0x04001954, 0x30701850,
0x00000000, 0x00b5841b, 0x0836186c, 0x283007f8,
0x00000000, 0x0095a000, 0x34001954, 0x383007f8,
0x00000000, 0x00200000, 0x3436106c, 0x20708858,
0x00000000, 0x0035a0aa, 0x0c36106c, 0x20704858,
0x00000000, 0x002c0055, 0x0c36106c, 0x20702858,
0x00000000, 0x002c0000, 0x0c36106c, 0x20701858,
0x00000000, 0x00200000, 0x3436106c, 0x20708860,
0x00000000, 0x0035a0ff, 0x0c36106c, 0x20704860,
0x00000000, 0x002c0055, 0x0c36106c, 0x20702860,
0x00000000, 0x002c0000, 0x0c36106c, 0x20701860,
0x00000000, 0x002ca0ff, 0x0c36106c, 0x2070e820,
0x00000000, 0x002c0055, 0x0c36106c, 0x20701820,
0x00000000, 0x0020061b, 0x0836106c, 0x2fa007f8,
0x00000000, 0x014c86ff, 0x09fe186c, 0x21a007f8,
0x00000000, 0x0020001b, 0xa436106c, 0x2070f819,
