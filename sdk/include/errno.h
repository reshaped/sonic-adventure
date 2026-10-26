/* C runtime error codes, as far as the sources use them */
#ifndef ERRNO_H
#define ERRNO_H

#define ERANGE 1100
#define EDOM 1101
#define PTRERR 1106 /* a null FILE */
#define ETLN 1202 /* a number of more than 17 significant digits */
#define EEXP 1204 /* an exponent of more than three significant digits */
#define EEXPN 1206 /* a normalized exponent past 999 either way */
#define ENUM 1208 /* an argument that is not a number */
#define EFLOATO 1210 /* a float result that overflows */
#define EFLOATU 1220 /* a float result that underflows */
#define EDBLO 1250 /* a double result that overflows */
#define EDBLU 1260 /* a double result that underflows */
#define ELDBLO 1270 /* a long double result that overflows */
#define ELDBLU 1280 /* a long double result that underflows */
#define NOTOPN 1300 /* a FILE that is not open */
#define EBADF 1302 /* a FILE not open for the operation */
/* an invalid directive, or input that differs from a literal byte or %% of
 * the format */
#define ECSPEC 1304

extern volatile int _errno;
#define errno _errno

#endif
