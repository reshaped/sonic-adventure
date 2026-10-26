/* C runtime character classes, as far as the sources use them */
#ifndef CTYPE_H
#define CTYPE_H

/* character class bits; a character can have several */
#define _UPPER 0x01
#define _LOWER 0x02
#define _DIGIT 0x04
#define _SPACE 0x08
#define _PUNCT 0x10
#define _CNTRL 0x20
#define _WHITE 0x40
#define _HEX 0x80

/* the class bits of each of the 256 character codes */
extern const unsigned char _ctype[];

#define isdigit(c) (_ctype[(c) & 0xff] & _DIGIT)
int toupper(int c);

#endif
