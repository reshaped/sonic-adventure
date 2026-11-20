/* the adapter's sums are doubles, and the PSP's processor has none: its C
 * library takes some hundred instructions for each operation on one. the
 * adapter's are floats there, and the library's functions of them are the
 * floats' own. the library's headers are read first, as they declare both */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define double float
#define floor floorf
#define pow powf
#define sqrt sqrtf
