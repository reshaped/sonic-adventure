/* the steps ADX's float decoder adds for each 4-bit code */
#include <sg_xpt.h>

/* a code read as signed, times 65536 */
#define STEP(code) ((Float)((code) < 8 ? (code) : (code) - 16) * 65536.0F)
#define CODES                                                                  \
    STEP(0), STEP(1), STEP(2), STEP(3), STEP(4), STEP(5), STEP(6), STEP(7),    \
        STEP(8), STEP(9), STEP(10), STEP(11), STEP(12), STEP(13), STEP(14),    \
        STEP(15)
#define SIXTEEN(code)                                                          \
    STEP(code), STEP(code), STEP(code), STEP(code), STEP(code), STEP(code),    \
        STEP(code), STEP(code), STEP(code), STEP(code), STEP(code),            \
        STEP(code), STEP(code), STEP(code), STEP(code), STEP(code)

const Float AdxQtblFloat[16] = {CODES};

/* AdxQtblFloat0 holds the step of each byte's high code, and AdxQtblFloat1 that
 * of its low code */
const Float AdxQtblFloat0[256] = {
    SIXTEEN(0),  SIXTEEN(1),  SIXTEEN(2),  SIXTEEN(3),
    SIXTEEN(4),  SIXTEEN(5),  SIXTEEN(6),  SIXTEEN(7),
    SIXTEEN(8),  SIXTEEN(9),  SIXTEEN(10), SIXTEEN(11),
    SIXTEEN(12), SIXTEEN(13), SIXTEEN(14), SIXTEEN(15),
};
const Float AdxQtblFloat1[256] = {
    CODES, CODES, CODES, CODES, CODES, CODES, CODES, CODES,
    CODES, CODES, CODES, CODES, CODES, CODES, CODES, CODES,
};
