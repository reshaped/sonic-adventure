/* a C name as the target's assembler spells it: with __USER_LABEL_PREFIX__,
 * an underscore under COFF and nothing under ELF, so that an asm string's
 * names suit either */
#ifndef XBOX_ASMNAME_H
#define XBOX_ASMNAME_H

#define XBOX_STRING_(text) #text
#define XBOX_STRING(text) XBOX_STRING_(text)
#define ASM_NAME(name) XBOX_STRING(__USER_LABEL_PREFIX__) #name

#endif
