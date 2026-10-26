#include <ctype.h>

#define C _CNTRL
#define W (_CNTRL | _WHITE)
#define S (_SPACE | _WHITE)
#define P _PUNCT
#define D (_DIGIT | _HEX)
#define UX (_UPPER | _HEX)
#define U _UPPER
#define LX (_LOWER | _HEX)
#define L _LOWER

/* each ASCII character's classes. every byte above 0x7f has none */
const unsigned char _ctype[256] = {
    C, C,  C,  C,  C,  C,  C,  C, C, W, W, W, W, W, C, C, /* 0x00 */
    C, C,  C,  C,  C,  C,  C,  C, C, C, C, C, C, C, C, C, /* 0x10 */
    S, P,  P,  P,  P,  P,  P,  P, P, P, P, P, P, P, P, P, /* 0x20 */
    D, D,  D,  D,  D,  D,  D,  D, D, D, P, P, P, P, P, P, /* 0x30 */
    P, UX, UX, UX, UX, UX, UX, U, U, U, U, U, U, U, U, U, /* 0x40 */
    U, U,  U,  U,  U,  U,  U,  U, U, U, U, P, P, P, P, P, /* 0x50 */
    P, LX, LX, LX, LX, LX, LX, L, L, L, L, L, L, L, L, L, /* 0x60 */
    L, L,  L,  L,  L,  L,  L,  L, L, L, L, P, P, P, P, C, /* 0x70 */
};
