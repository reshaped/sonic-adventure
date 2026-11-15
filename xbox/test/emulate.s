! _harness_emulate: run xbox/test/emulate.c's harness_emulate on the
! registers as the caller left them, then return past the call's code. the
! caller pushed R0, then PR, and PR now points at the code halfword. every
! register but those the instruction writes comes back as it was, T among
! them. the registers live in harness_cpu while the C runs:
!   0 r0 to r7 (r0 unused), 32 pr, 36 t, 40 fpul, 44 macl, 48 mach,
!   52 fr0 to fr15, 116 xf0 to xf15
        .text
        .global _harness_emulate
        .align  2
_harness_emulate:
        mov.l   .Lcpu,r0
        mov.l   r1,@(4,r0)
        mov.l   r2,@(8,r0)
        mov.l   r3,@(12,r0)
        mov.l   r4,@(16,r0)
        mov.l   r5,@(20,r0)
        mov.l   r6,@(24,r0)
        mov.l   r7,@(28,r0)
        sts     pr,r1
        mov.l   r1,@(32,r0)
        movt    r1
        mov.l   r1,@(36,r0)
        sts     fpul,r1
        mov.l   r1,@(40,r0)
        sts     macl,r1
        mov.l   r1,@(44,r0)
        sts     mach,r1
        mov.l   r1,@(48,r0)
        mov     r0,r1
        add     #116,r1                 ! past fr15
        fmov.s  fr15,@-r1
        fmov.s  fr14,@-r1
        fmov.s  fr13,@-r1
        fmov.s  fr12,@-r1
        fmov.s  fr11,@-r1
        fmov.s  fr10,@-r1
        fmov.s  fr9,@-r1
        fmov.s  fr8,@-r1
        fmov.s  fr7,@-r1
        fmov.s  fr6,@-r1
        fmov.s  fr5,@-r1
        fmov.s  fr4,@-r1
        fmov.s  fr3,@-r1
        fmov.s  fr2,@-r1
        fmov.s  fr1,@-r1
        fmov.s  fr0,@-r1
        frchg
        mov     r0,r1
        add     #127,r1
        add     #53,r1                  ! past xf15, at 180
        fmov.s  fr15,@-r1
        fmov.s  fr14,@-r1
        fmov.s  fr13,@-r1
        fmov.s  fr12,@-r1
        fmov.s  fr11,@-r1
        fmov.s  fr10,@-r1
        fmov.s  fr9,@-r1
        fmov.s  fr8,@-r1
        fmov.s  fr7,@-r1
        fmov.s  fr6,@-r1
        fmov.s  fr5,@-r1
        fmov.s  fr4,@-r1
        fmov.s  fr3,@-r1
        fmov.s  fr2,@-r1
        fmov.s  fr1,@-r1
        fmov.s  fr0,@-r1
        frchg
        mov.l   @(32,r0),r1
        mov.w   @r1,r4                  ! the call's code
        mov     r0,r5
        mov.l   .Lc,r1
        jsr     @r1
        nop
        mov.l   .Lcpu,r0
        frchg
        mov     r0,r1
        add     #116,r1
        fmov.s  @r1+,fr0
        fmov.s  @r1+,fr1
        fmov.s  @r1+,fr2
        fmov.s  @r1+,fr3
        fmov.s  @r1+,fr4
        fmov.s  @r1+,fr5
        fmov.s  @r1+,fr6
        fmov.s  @r1+,fr7
        fmov.s  @r1+,fr8
        fmov.s  @r1+,fr9
        fmov.s  @r1+,fr10
        fmov.s  @r1+,fr11
        fmov.s  @r1+,fr12
        fmov.s  @r1+,fr13
        fmov.s  @r1+,fr14
        fmov.s  @r1+,fr15
        frchg
        mov     r0,r1
        add     #52,r1
        fmov.s  @r1+,fr0
        fmov.s  @r1+,fr1
        fmov.s  @r1+,fr2
        fmov.s  @r1+,fr3
        fmov.s  @r1+,fr4
        fmov.s  @r1+,fr5
        fmov.s  @r1+,fr6
        fmov.s  @r1+,fr7
        fmov.s  @r1+,fr8
        fmov.s  @r1+,fr9
        fmov.s  @r1+,fr10
        fmov.s  @r1+,fr11
        fmov.s  @r1+,fr12
        fmov.s  @r1+,fr13
        fmov.s  @r1+,fr14
        fmov.s  @r1+,fr15
        mov.l   @(40,r0),r1
        lds     r1,fpul
        mov.l   @(44,r0),r1
        lds     r1,macl
        mov.l   @(48,r0),r1
        lds     r1,mach
        mov.l   @(32,r0),r1             ! past the code and the literal
        add     #5,r1
        mov     #-4,r2
        and     r2,r1
        add     #4,r1
        lds     r1,pr
        mov.l   @(36,r0),r1
        shlr    r1                      ! T as the caller left it
        mov.l   @(4,r0),r1
        mov.l   @(8,r0),r2
        mov.l   @(12,r0),r3
        mov.l   @(16,r0),r4
        mov.l   @(20,r0),r5
        mov.l   @(24,r0),r6
        rts
        mov.l   @(28,r0),r7
        .align  2
.Lcpu:
        .long   _harness_cpu
.Lc:
        .long   _harness_emulate_c
