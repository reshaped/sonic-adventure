/* setjmp for the port: make pc compiles this in place of
 * sdk/runtime/setjmp.src. the context keeps the SH-4's word for each
 * register where sdk/shinobi/rfiber.c reaches into it: word 0 the stack
 * pointer, as R15, and word 2 the return address, as PR. word 1 holds ebp,
 * as R14, then ebx, esi and edi. the stack pointer is the one setjmp's
 * caller has once setjmp returns */
#include <setjmp.h>

__asm__(".text\n"
        ".globl setjmp\n"
        "setjmp:\n"
        "    movl 4(%esp), %eax\n"
        "    leal 4(%esp), %ecx\n"
        "    movl %ecx, 0(%eax)\n"
        "    movl %ebp, 4(%eax)\n"
        "    movl (%esp), %ecx\n"
        "    movl %ecx, 8(%eax)\n"
        "    movl %ebx, 12(%eax)\n"
        "    movl %esi, 16(%eax)\n"
        "    movl %edi, 20(%eax)\n"
        "    xorl %eax, %eax\n"
        "    ret\n");
