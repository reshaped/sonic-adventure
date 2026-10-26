#include <ninja.h>
#include <sg_xpt.h>

void njMemCopy4(void *dst, const void *src, Sint32 num)
{
    do {
        /* @hack redundant: the empty switch on the copied long steers it into
         * R0 rather than R3; without it, 2 of 7 words differ */
        switch (*(Uint32 *)dst = *(const Uint32 *)src) {
        }
        src = (const Uint32 *)src + 1;
        dst = (Uint32 *)dst + 1;
    } while (--num != 0);
}

void njMemCopy2(void *dst, const void *src, Sint32 num)
{
    do {
        /* @hack redundant: the empty switch on the copied half steers it into
         * R0 rather than R3; without it, 2 of 7 words differ */
        switch (*(Sint16 *)dst = *(const Sint16 *)src) {
        }
        src = (const Sint16 *)src + 1;
        dst = (Sint16 *)dst + 1;
    } while (--num != 0);
}

void njMemCopy(void *dst, const void *src, Sint32 num)
{
    do {
        /* @hack redundant: the empty switch on the copied byte steers it into
         * R0 rather than R3; without it, 2 of 7 words differ */
        switch (*(Sint8 *)dst = *(const Sint8 *)src) {
        }
        src = (const Sint8 *)src + 1;
        dst = (Sint8 *)dst + 1;
    } while (--num != 0);
}
