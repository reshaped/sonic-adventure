/* longjmp for the port: make pc compiles this in place of
 * sdk/runtime/longjmp.src. it loads the context xbox/sdk/runtime/setjmp.c
 * saves, then returns to its word 2 on its word 0's stack with val, or with
 * 1 for a val of 0. rfiber.c's fibers start so: word 0 a new stack's top,
 * word 2 the fiber's function */
#include <setjmp.h>

__asm__(".text\n"
        ".globl longjmp\n"
        "longjmp:\n"
        "    movl 4(%esp), %edx\n"
        "    movl 8(%esp), %eax\n"
        "    testl %eax, %eax\n"
        "    jnz 1f\n"
        "    movl $1, %eax\n"
        "1:\n"
        "    movl 4(%edx), %ebp\n"
        "    movl 12(%edx), %ebx\n"
        "    movl 16(%edx), %esi\n"
        "    movl 20(%edx), %edi\n"
        "    movl 0(%edx), %esp\n"
        "    jmp *8(%edx)\n");
