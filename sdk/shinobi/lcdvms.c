/* VMU LCD image writes */
#include <kaede.h>
#include <sg_lcd.h>
#include <sg_xpt.h>
#include <shinobi.h>

/* how many LCDs pdgLcdNo numbers: one in each of the first two expansion
 * sockets of the four controllers */
#define LCDS 8
/* the bytes of an LCD image, 48 by 32 pixels at one bit each.
 * kdSetBlockWrite takes its size in 4-byte words, LCD_WORDS */
#define LCD_SIZE (48 * 32 / 8)
#define LCD_WORDS (LCD_SIZE / 4)
/* a VMU's function bits: storage, LCD and timer */
#define VMS_TYPE (PDD_DEVTYPE_STORAGE | PDD_DEVTYPE_LCD | PDD_DEVTYPE_TIMER)

/* lcdasm's image copies, unflipped and flipped each way */
extern void pdVmsLcdBltAsm(void *dst, void *src);
extern void pdVmsLcdBltAsmFlipH(void *dst, void *src);
extern void pdVmsLcdBltAsmFlipV(void *dst, void *src);
extern void pdVmsLcdBltAsmFlipHV(void *dst, void *src);

/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const char lcdvms_build[] = "pdgLcd Build:Sep 07 1998 11:53:36";

/* each LCD's image as pdVmsLcdWrite last packed it, flipped as its flag
 * said */
Uint8 pdgVmsWork[LCDS][LCD_SIZE];

void pdVmsLcdBlt(void *dst, void *src, Uint32 flag);

Sint32 pdVmsLcdWrite(Uint32 port, void *data, Uint32 flag)
{
    Sint32 err;
    /* @hack spelling: the word count through words, which the image keeps at
     * @(4,R15) and pushes from there (0x8c61d176 to 0x8c61d17e); passed as
     * LCD_WORDS, the function runs 4 bytes short and 40 of 64 words differ */
    Sint32 words;
    Uint8 *work;

    if (!pdVmsLcdIsReady(port)) {
        return PDD_LCDERR_NO_LCD;
    }
    work = pdgVmsWork[pdgLcdNo[port]];
    pdVmsLcdBlt(work, data, flag);
    words = LCD_WORDS;
    err = kdSetBlockWrite(port, PDD_DEVTYPE_LCD, 0, 0, 0, work, words);
    if (err < 0) {
        return PDD_LCDERR_BUSY;
    }
    return PDD_LCDERR_OK;
}

Sint32 pdVmsLcdWrite1(Uint32 port, void *data)
{
    Sint32 err;
    /* @hack spelling: the word count through words, which the image keeps at
     * @R15 and pushes from there (0x8c61d1e2 to 0x8c61d1e6); passed as
     * LCD_WORDS, the function runs 4 bytes short and 34 of 44 words differ */
    Sint32 words;

    if (!pdVmsLcdIsReady(port)) {
        return PDD_LCDERR_NO_LCD;
    }
    words = LCD_WORDS;
    err = kdSetBlockWrite(port, PDD_DEVTYPE_LCD, 0, 0, 0, data, words);
    if (err < 0) {
        return PDD_LCDERR_BUSY;
    }
    return PDD_LCDERR_OK;
}

Sint32 pdVmsLcdIsReady(Uint32 port)
{
    const KDS_DEVICEINFO *info;
    KDS_LCDINFO lcd;
    Sint32 err;

    info = kdGetDeviceInfo(port);
    err = kdDevGetLCDInfo(&lcd, info);
    if (err < 0) {
        return false;
    }
    if (info->function != VMS_TYPE) {
        return false;
    }
    if (lcd.partition != 1) {
        return false;
    }
    if (lcd.blocksize != LCD_SIZE) {
        return false;
    }
    if (lcd.writecount != 1) {
        return false;
    }
    if (lcd.direction != KDD_LCD_HV_00) {
        return false;
    }
    if (lcd.normally != KDD_LCD_NORMALLYWHITE) {
        return false;
    }
    return true;
}

void pdVmsLcdBlt(void *dst, void *src, Uint32 flag)
{
    switch (flag) {
    case PDD_LCD_FLAG_HFLIP:
        pdVmsLcdBltAsmFlipH(dst, src);
        break;
    case PDD_LCD_FLAG_VFLIP:
        pdVmsLcdBltAsmFlipV(dst, src);
        break;
    case PDD_LCD_FLAG_HVFLIP:
        pdVmsLcdBltAsmFlipHV(dst, src);
        break;
    default:
        pdVmsLcdBltAsm(dst, src);
        break;
    }
}
