/* Sofdec's comparisons of 64-bit numbers, each a high and a low word */
#include <sg_xpt.h>

#include "movie/sdk/sfd.h"

Bool UTY_CmpEqUint64(struct uty_uint64 *a, struct uty_uint64 *b)
{
    /* @hack spelling: the result through result, which SHC builds in R4 and
     * copies to R0 in the RTS's slot (MOV R4,R0 at 0x8ced95e0); returned in
     * each branch, 4 of 13 words differ */
    Bool result;

    if (a->hi == b->hi && a->lo == b->lo) {
        result = TRUE;
    } else {
        result = FALSE;
    }
    return result;
}

Bool UTY_CmpEqSint64(struct uty_sint64 *a, struct uty_sint64 *b)
{
    /* @hack spelling: the result through result, as in UTY_CmpEqUint64 (MOV
     * R4,R0 at 0x8ced95fa); returned in each branch, 4 of 13 words differ */
    Bool result;

    if (a->hi == b->hi && a->lo == b->lo) {
        result = TRUE;
    } else {
        result = FALSE;
    }
    return result;
}

Bool UTY_CmpLeUint64(struct uty_uint64 *a, struct uty_uint64 *b)
{
    /* @hack spelling: the result through result, as in UTY_CmpEqUint64 (MOV
     * R4,R0 at 0x8ced9620); returned in each branch, 4 of 19 words differ */
    Bool result;

    if (a->hi == b->hi) {
        if (a->lo <= b->lo) {
            result = TRUE;
        } else {
            result = FALSE;
        }
    } else if (a->hi <= b->hi) {
        result = TRUE;
    } else {
        result = FALSE;
    }
    return result;
}

Bool UTY_CmpLeSint64(struct uty_sint64 *a, struct uty_sint64 *b)
{
    /* @hack spelling: the result through result, as in UTY_CmpEqUint64 (MOV
     * R4,R0 at 0x8ced9658); returned in each branch, 7 of 28 words differ */
    Bool result;

    if (a->hi == b->hi) {
        /* @bug logic: where the high words are equal and below 0, the low
         * words compare in reverse (CMP/HS R0,R1 at 0x8ced9640). UTY_Dmuls
         * stores two's complement, whose low words order the same at either
         * sign, so -2 <= -1 comes out FALSE */
        if (a->hi >= 0) {
            if (a->lo <= b->lo) {
                result = TRUE;
            } else {
                result = FALSE;
            }
        } else if (a->lo >= b->lo) {
            result = TRUE;
        } else {
            result = FALSE;
        }
    } else if (a->hi <= b->hi) {
        result = TRUE;
    } else {
        result = FALSE;
    }
    return result;
}

Bool UTY_CmpLtUint64(struct uty_uint64 *a, struct uty_uint64 *b)
{
    /* @hack spelling: the result through result, as in UTY_CmpEqUint64 (MOV
     * R4,R0 at 0x8ced967e); returned in each branch, 4 of 19 words differ */
    Bool result;

    if (a->hi == b->hi) {
        if (a->lo < b->lo) {
            result = TRUE;
        } else {
            result = FALSE;
        }
    } else if (a->hi < b->hi) {
        result = TRUE;
    } else {
        result = FALSE;
    }
    return result;
}

Bool UTY_CmpLtSint64(struct uty_sint64 *a, struct uty_sint64 *b)
{
    /* @hack spelling: the result through result, as in UTY_CmpEqUint64 (MOV
     * R4,R0 at 0x8ced96b6); returned in each branch, 7 of 28 words differ */
    Bool result;

    if (a->hi == b->hi) {
        /* @bug logic: where the high words are equal and below 0, the low
         * words compare in reverse, as in UTY_CmpLeSint64 (CMP/HI R0,R1 at
         * 0x8ced969e), so -2 < -1 comes out FALSE */
        if (a->hi >= 0) {
            if (a->lo < b->lo) {
                result = TRUE;
            } else {
                result = FALSE;
            }
        } else if (a->lo > b->lo) {
            result = TRUE;
        } else {
            result = FALSE;
        }
    } else if (a->hi < b->hi) {
        result = TRUE;
    } else {
        result = FALSE;
    }
    return result;
}

Bool UTY_CmpPlSint64(struct uty_sint64 *a)
{
    /* @hack spelling: the result through result, which SHC builds in R5 and
     * copies to R0 in the RTS's slot (MOV R5,R0 at 0x8ced96d0); returned in
     * each branch, 11 of 13 words differ */
    Bool result;

    result = FALSE;
    if (a->hi > 0) {
        result = TRUE;
    } else if (a->hi == 0 && a->lo != 0) {
        result = TRUE;
    }
    return result;
}

Bool UTY_CmpPzSint64(struct uty_sint64 *a)
{
    Bool result;

    /* @hack spelling: the test through result, which SHC sets in R4 and
     * copies to R0 in the RTS's slot (MOVT R4 and MOV R4,R0 at 0x8ced96d6
     * and 0x8ced96da); returned straight, 3 of 5 words differ */
    result = a->hi >= 0;
    return result;
}
