/* C runtime variable argument lists, as far as the sources use them */
#ifndef STDARG_H
#define STDARG_H

#ifdef GCC
/* SHC passes param and the arguments after it on the stack, where the
 * #else side's walk reads them. GCC passes them in registers while any are
 * free */
typedef __builtin_va_list va_list;

#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)
#else
typedef char *va_list;

/* point ap at the argument after param. each argument takes whole four-byte
 * slots */
#define va_start(ap, param)                                                    \
    (void)((ap) = (int)((char *)&(param) + sizeof(param)) % 4U                 \
                      ? (char *)&(param) + (((sizeof(param) / 4U) + 1U) * 4U)  \
                      : (char *)&(param) + sizeof(param))
/* the next argument, as type, with ap advanced past its slots */
#define va_arg(ap, type)                                                       \
    ((ap) = (int)((ap) + sizeof(type)) % 4U                                    \
                ? (ap) + (((sizeof(type) / 4U) + 1U) * 4U)                     \
                : (ap) + sizeof(type),                                         \
     *(type *)((int)((ap) - sizeof(type)) % 4U                                 \
                   ? (ap) - (((sizeof(type) / 4U) + 1U) * 4U)                  \
                   : (ap) - sizeof(type)))
#define va_end(ap)
#endif

#endif
