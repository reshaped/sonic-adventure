/* the memory card file system's long word copy and its 16-bit fields, for
 * the port: make pc compiles this in place of sdk/shinobi/vmsasm.src */
#include <sg_xpt.h>

void vmsfs_memcpyl(void *dst, const void *src, Sint32 size)
{
    Uint32 *to = dst;
    const Uint32 *from = src;
    Uint32 count = (Uint32)size / 4;

    while (count-- > 0) {
        *to++ = *from++;
    }
}

Uint16 vmsfs_getword_big(void *buf, Sint32 offset)
{
    const Uint8 *at = (const Uint8 *)buf + offset;

    return (Uint16)((at[0] << 8) | at[1]);
}

Uint16 vmsfs_getword_little(void *buf, Sint32 offset)
{
    const Uint8 *at = (const Uint8 *)buf + offset;

    return (Uint16)(at[0] | (at[1] << 8));
}

void vmsfs_setword_big(void *buf, Sint32 offset, Uint16 value)
{
    Uint8 *at = (Uint8 *)buf + offset;

    at[0] = (Uint8)(value >> 8);
    at[1] = (Uint8)value;
}

void vmsfs_setword_little(void *buf, Sint32 offset, Uint16 value)
{
    Uint8 *at = (Uint8 *)buf + offset;

    at[0] = (Uint8)value;
    at[1] = (Uint8)(value >> 8);
}
