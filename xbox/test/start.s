! the SH-4 harness's entry under qemu-sh4: the console's FPU mode, then
! main, then exit with its status. FPSCR 0x00040001 is single precision,
! denormals as zero and rounding toward zero, as the game runs
        .text
        .global _start
_start:
        mov.l   fpscr_value,r0
        lds     r0,fpscr
        mov.l   main_address,r0
        jsr     @r0
        nop
        mov     r0,r4
        mov     #1,r3                   ! exit
        trapa   #0x11
        .align  2
fpscr_value:
        .long   0x00040001
main_address:
        .long   _main
