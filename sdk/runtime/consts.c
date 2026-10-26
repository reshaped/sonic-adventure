/* float constants for the math members, defined by their bits: C has no
 * literal for a NaN or an infinity. the members declare them as float */

const unsigned long _IFNANS = 0x7f800001; /* a NaN */
const unsigned long _IFZEROS = 0x00000000; /* 0 */
const unsigned long _IFINFS = 0x7f800000; /* infinity */
const unsigned long _IFONES = 0x3f800000; /* 1 */
const unsigned long _HALFS = 0x3f000000; /* a half */
const unsigned long _QUOTS = 0x3e800000; /* a quarter */
const unsigned long _TWOS = 0x40000000; /* 2 */
const unsigned long _FOURS = 0x40800000; /* 4 */
const unsigned long _PAIS = 0x40490fdb; /* pi */
const unsigned long _2PAIS = 0x40c90fdb; /* 2 pi */
const unsigned long _HLFPAIS = 0x3fc90fdb; /* pi over 2 */
const unsigned long _QUOPAIS = 0x3f490fdb; /* pi over 4 */
/* the natural log of 2 */
const unsigned long _LOGE2S = 0x3f317218;

/* the same constants negated, except the NaN */
const unsigned long _NIFZEROS = 0x80000000;
const unsigned long _NIFINFS = 0xff800000;
const unsigned long _NIFONES = 0xbf800000;
const unsigned long _NHALFS = 0xbf000000;
const unsigned long _NQUOTS = 0xbe800000;
const unsigned long _NTWOS = 0xc0000000;
const unsigned long _NFOURS = 0xc0800000;
const unsigned long _NPAIS = 0xc0490fdb;
const unsigned long _N2PAIS = 0xc0c90fdb;
const unsigned long _NHLFPAIS = 0xbfc90fdb;
const unsigned long _NQUOPAIS = 0xbf490fdb;
const unsigned long _NLOGE2S = 0xbf317218;
