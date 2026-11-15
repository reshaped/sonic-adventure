/* the port's stand-in for Flycast's core/build.h: the clocks, the ARM7
 * interpreted, and the DSP recompiled for 32-bit x86 (dsp_x86.cpp), unless
 * the build defines FLYCAST_DSP_INTERPRETER */
#pragma once
#define DYNAREC_NONE 0x40000001
#define DYNAREC_JIT 0x40000002
#define FEAT_AREC DYNAREC_NONE
#ifdef FLYCAST_DSP_INTERPRETER
#define FEAT_DSPREC DYNAREC_NONE
#else
#define FEAT_DSPREC DYNAREC_JIT
#endif
#define CPU_X86 0x20000001
#define HOST_CPU CPU_X86
#define SH4_MAIN_CLOCK (200 * 1000 * 1000)
#define AICA_CORE_CLOCK 45158400 /* 1024 fs */
#define AICA_ARM_CLOCK (AICA_CORE_CLOCK / 2)
