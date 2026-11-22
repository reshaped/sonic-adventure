/* libmpeg2 0.5.1's configuration for the port (configure's config.h.in, by
 * hand): x86, the acceleration given, not detected (xbox/port/movie.c calls
 * mpeg2_accel with MMX and MMXEXT, the Pentium III's, which choose the
 * MMXEXT IDCT and motion compensation), and the SDK's C library, which the
 * port compiles against freestanding.
 *
 * the port takes the decoder alone, libmpeg2/'s and include/'s files of the
 * release in one folder, unchanged but for this file; its colorspace
 * conversion, libmpeg2convert, the host does (host_api's
 * video_movie_frame). libmpeg2 is GPL-2.0, as the port is (COPYING) */
#ifndef LIBMPEG2_PORT_CONFIG_H
#define LIBMPEG2_PORT_CONFIG_H

/* NULL, which the SDK's stdlib.h leaves to stddef.h */
#include <stddef.h>

#define ARCH_X86 1
#define HAVE_BUILTIN_EXPECT 1
#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define STDC_HEADERS 1
#define ATTRIBUTE_ALIGNED_MAX 64
#define SIZEOF_CHAR 1
#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG 4
#define SIZEOF_VOIDP 4

#endif
