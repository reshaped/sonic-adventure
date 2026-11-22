#include "arm7.h"
#include "arm_mem.h"
#include "arm7_rec.h"
#ifdef FLYCAST_AICA_ONLY
#include "hw/aica/aica.h"
#endif

namespace aica::arm
{

#define CPUReadMemoryQuick(addr) (*(u32*)&aica_ram[(addr) & ARAM_MASK])
#define CPUReadByte readMem<u8>
#define CPUReadMemory readMem<u32>
#define CPUReadHalfWord readMem<u16>
#define CPUReadHalfWordSigned(addr) ((s16)readMem<u16>(addr))

#define CPUWriteMemory writeMem<u32>
#define CPUWriteHalfWord writeMem<u16>
#define CPUWriteByte writeMem<u8>

#define reg arm_Reg
#define armNextPC reg[R15_ARM_NEXT].I

#define CPUUpdateTicksAccessSeq32(a) 1
#define CPUUpdateTicksAccess32(a) 1
#define CPUUpdateTicksAccess16(a) 1

alignas(8) reg_pair arm_Reg[RN_ARM_REG_COUNT];

static void CPUSwap(u32 *a, u32 *b)
{
	u32 c = *b;
	*b = *a;
	*a = c;
}

#define N_FLAG (reg[RN_PSR_FLAGS].FLG.N)
#define Z_FLAG (reg[RN_PSR_FLAGS].FLG.Z)
#define C_FLAG (reg[RN_PSR_FLAGS].FLG.C)
#define V_FLAG (reg[RN_PSR_FLAGS].FLG.V)

bool armIrqEnable;
bool armFiqEnable;
int armMode;

bool Arm7Enabled = false;

u8 cpuBitsSet[256];

static void CPUSwitchMode(int mode, bool saveState);
static void CPUUpdateFlags();
static void CPUSoftwareInterrupt(int comment);
static void CPUUndefinedException();

//
// ARM7 interpreter
//
int arm7ClockTicks;
#ifdef FLYCAST_AICA_ONLY
u32 idle_pc = ~0u;
bool (*idle_wait)();
#endif

#if FEAT_AREC == DYNAREC_NONE

/* one instruction, as the interpreter runs it */
#ifdef FLYCAST_TRACE_PC
extern "C" void flycast_pc(unsigned pc, unsigned r0, unsigned r1, unsigned flags);
#define TRACE_PC(pc) flycast_pc(pc, reg[0].I, reg[1].I, reg[RN_PSR_FLAGS].I);
#else
#define TRACE_PC(pc)
#endif

static inline void interpretOne()
{
	TRACE_PC(armNextPC)
	reg[15].I = armNextPC + 8;

	int& clockTicks = arm7ClockTicks;
	#include "arm-new.h"
}

#ifdef ARM_STATIC
/* the port: the sound driver's code translated ahead (tools/arm2c.py's
 * driver.inc), which runs in the interpreter's place while sound RAM holds
 * that driver. an instruction is the interpreter's own code for it,
 * arm-new.h, with the opcode a constant and the decode's index a template's,
 * so that the compiler leaves the one case and folds the fields: staticOp.
 * runStatic strings them in the driver's order between the interpreter's
 * own checks, of the ticks left, of a pending interrupt and of the idle
 * loop, so a run is the interpreter's, tick for tick; an address the
 * translation lacks is interpreted (staticMissed counts them) */
static bool staticValid;
unsigned staticMissed;
#ifdef FLYCAST_TRACE
extern "C" void flycast_static_missed(unsigned pc);
#endif

#define NO_OPCODE_READ
template <unsigned HASH>
static inline __attribute__((always_inline)) void staticOp(const u32 opcode)
{
	int& clockTicks = arm7ClockTicks;
#define ARM_STATIC_HASH HASH
	#include "arm-new.h"
#undef ARM_STATIC_HASH
}
#undef NO_OPCODE_READ

static void staticCheck()
{
	/* the instructions' words, where sound RAM has them */
	static const u16 places[] = {
#define ARM_IMAGE(image_size, image_sum)
#define ARM_OP(addr, op, hash, next) addr,
#define ARM_IDLE(addr, op, hash, next) addr,
	#include "driver.inc"
#undef ARM_IMAGE
#undef ARM_OP
#undef ARM_IDLE
	};
	u32 sum = 2166136261u;
	u32 want = 0;
#define ARM_IMAGE(image_size, image_sum) want = image_sum;
#define ARM_OP(addr, op, hash, next)
#define ARM_IDLE(addr, op, hash, next)
	#include "driver.inc"
#undef ARM_IMAGE
#undef ARM_OP
#undef ARM_IDLE
	for (u16 at : places)
		sum = (sum ^ *(u32 *)&aica_ram[at]) * 16777619u;
	staticValid = sum == want;
}

static void runStatic()
{
	while (arm7ClockTicks < 0)
	{
		switch (armNextPC)
		{
#define ARM_IMAGE(image_size, image_sum)
#define ARM_BEFORE(addr) \
		case addr: \
			if (arm7ClockTicks >= 0) \
				return; \
			if (reg[INTR_PEND].I) \
			{ \
				CPUFiq(); \
				break; \
			}
#define ARM_RUN(addr, op, hash, next) \
			TRACE_PC(addr) \
			reg[15].I = addr + 8; \
			armNextPC = addr + 4; \
			staticOp<hash>(op); \
			if (!next || armNextPC != addr + 4) \
				break;
#define ARM_OP(addr, op, hash, next) ARM_BEFORE(addr) ARM_RUN(addr, op, hash, next)
#define ARM_IDLE(addr, op, hash, next) \
		ARM_BEFORE(addr) \
			if (idle_pc == addr && idle_wait != nullptr && idle_wait()) \
			{ \
				arm7ClockTicks = 0; \
				return; \
			} \
		ARM_RUN(addr, op, hash, next)
		#include "driver.inc"
#undef ARM_IMAGE
#undef ARM_OP
#undef ARM_IDLE
			break;
		default:
			if (reg[INTR_PEND].I)
			{
				CPUFiq();
				break;
			}
			if (armNextPC == idle_pc && idle_wait != nullptr && idle_wait())
			{
				arm7ClockTicks = 0;
				return;
			}
			staticMissed++;
#ifdef FLYCAST_TRACE
			flycast_static_missed(armNextPC);
#endif
			interpretOne();
			break;
		}
	}
}
#endif

static void runInterpreter(u32 CycleCount)
{
	if (!Arm7Enabled)
		return;

	arm7ClockTicks -= CycleCount;
#ifdef ARM_STATIC
	{
		/* the driver may be another, or not all there yet: looked at again
		 * every tenth of a second */
		static u32 since;

		if (since++ % 4096 == 0)
			staticCheck();
		if (staticValid)
		{
			runStatic();
			return;
		}
	}
#endif
	while (arm7ClockTicks < 0)
	{
		if (reg[INTR_PEND].I)
			CPUFiq();
#ifdef FLYCAST_AICA_ONLY
		/* the port: at the driver's idle loop's head, with nothing for it to
		 * find (idle_wait, xbox/host/pc/sound.cpp's), the rest of the sample
		 * passes at once. what the loop polls changes only between samples,
		 * at an interrupt or a write of the SH-4's, so the sample's output is
		 * the same */
		if (armNextPC == idle_pc && !reg[INTR_PEND].I && idle_wait != nullptr && idle_wait())
		{
			arm7ClockTicks = 0;
			break;
		}
#endif

		interpretOne();
	}
}

void avoidRaceCondition()
{
	arm7ClockTicks = std::min(arm7ClockTicks, -50);
}

#if defined(FLYCAST_AICA_ONLY) && defined(FLYCAST_PROFILE)
/* the port: what the ARM7 and the sample's step take, in the CPU's
 * timestamp counts, for the profile */
unsigned long long profile_arm, profile_step;
#endif

void run(u32 samples)
{
	for (u32 i = 0; i < samples; i++)
	{
#if defined(FLYCAST_AICA_ONLY) && !defined(FLYCAST_NO_QUIET)
		/* the port: with the ARM7 off, or at the driver's idle loop with
		 * nothing for it there, it does nothing in a sample, and nothing
		 * until an interrupt: the samples to the next that may raise one
		 * pass in one step (aica.cpp's timeStepQuiet) */
		if (!Arm7Enabled || (armNextPC == idle_pc && !reg[INTR_PEND].I && idle_wait != nullptr && idle_wait()))
		{
			u32 quiet = std::min(samples - i, aica::quietSamples());

			if (quiet > 1)
			{
				if (Arm7Enabled)
					arm7ClockTicks = 0;
				aica::timeStepQuiet(quiet);
				i += quiet - 1;
				continue;
			}
		}
#endif
#if defined(FLYCAST_AICA_ONLY) && defined(FLYCAST_PROFILE)
		unsigned long long t0 = __builtin_ia32_rdtsc();
		runInterpreter(ARM_CYCLES_PER_SAMPLE);
		unsigned long long t1 = __builtin_ia32_rdtsc();
		{
			/* the port: where the ARM7 is at each sample, for the profile */
			extern unsigned profile_pcs[];
			profile_pcs[(reg[15].I >> 2) & 0x7fff]++;
		}
		timeStep();
		profile_arm += t1 - t0;
		profile_step += __builtin_ia32_rdtsc() - t1;
#else
		runInterpreter(ARM_CYCLES_PER_SAMPLE);
		timeStep();
#endif
	}
}
#endif

void staticInit()
{
	for (std::size_t i = 0; i < std::size(cpuBitsSet); i++)
	{
		int count = 0;
		for (int j = 0; j < 8; j++)
			if (i & (1 << j))
				count++;

		cpuBitsSet[i] = count;
	}
}
OnLoad _staticInit(staticInit);

void init()
{
#if FEAT_AREC != DYNAREC_NONE
	recompiler::init();
#endif
	reset();
}

void term()
{
#if FEAT_AREC != DYNAREC_NONE
	recompiler::term();
#endif
}

static void CPUSwitchMode(int mode, bool saveState)
{
	CPUUpdateCPSR();

	switch(armMode)
	{
	case 0x10:
	case 0x1F:
		reg[R13_USR].I = reg[13].I;
		reg[R14_USR].I = reg[14].I;
		reg[RN_SPSR].I = reg[RN_CPSR].I;
		break;
	case 0x11:
		CPUSwap(&reg[R8_FIQ].I, &reg[8].I);
		CPUSwap(&reg[R9_FIQ].I, &reg[9].I);
		CPUSwap(&reg[R10_FIQ].I, &reg[10].I);
		CPUSwap(&reg[R11_FIQ].I, &reg[11].I);
		CPUSwap(&reg[R12_FIQ].I, &reg[12].I);
		reg[R13_FIQ].I = reg[13].I;
		reg[R14_FIQ].I = reg[14].I;
		reg[SPSR_FIQ].I = reg[RN_SPSR].I;
		break;
	case 0x12:
		reg[R13_IRQ].I  = reg[13].I;
		reg[R14_IRQ].I  = reg[14].I;
		reg[SPSR_IRQ].I =  reg[RN_SPSR].I;
		break;
	case 0x13:
		reg[R13_SVC].I  = reg[13].I;
		reg[R14_SVC].I  = reg[14].I;
		reg[SPSR_SVC].I =  reg[RN_SPSR].I;
		break;
	case 0x17:
		reg[R13_ABT].I  = reg[13].I;
		reg[R14_ABT].I  = reg[14].I;
		reg[SPSR_ABT].I =  reg[RN_SPSR].I;
		break;
	case 0x1b:
		reg[R13_UND].I  = reg[13].I;
		reg[R14_UND].I  = reg[14].I;
		reg[SPSR_UND].I =  reg[RN_SPSR].I;
		break;
	}

	u32 CPSR = reg[RN_CPSR].I;
	u32 SPSR = reg[RN_SPSR].I;

	switch(mode)
	{
	case 0x10:
	case 0x1F:
		reg[13].I = reg[R13_USR].I;
		reg[14].I = reg[R14_USR].I;
		reg[RN_CPSR].I = SPSR;
		break;
	case 0x11:
		CPUSwap(&reg[8].I, &reg[R8_FIQ].I);
		CPUSwap(&reg[9].I, &reg[R9_FIQ].I);
		CPUSwap(&reg[10].I, &reg[R10_FIQ].I);
		CPUSwap(&reg[11].I, &reg[R11_FIQ].I);
		CPUSwap(&reg[12].I, &reg[R12_FIQ].I);
		reg[13].I = reg[R13_FIQ].I;
		reg[14].I = reg[R14_FIQ].I;
		if(saveState)
			reg[RN_SPSR].I = CPSR;
		else
			reg[RN_SPSR].I = reg[SPSR_FIQ].I;
		break;
	case 0x12:
		reg[13].I = reg[R13_IRQ].I;
		reg[14].I = reg[R14_IRQ].I;
		reg[RN_CPSR].I = SPSR;
		if(saveState)
			reg[RN_SPSR].I = CPSR;
		else
			reg[RN_SPSR].I = reg[SPSR_IRQ].I;
		break;
	case 0x13:
		reg[13].I = reg[R13_SVC].I;
		reg[14].I = reg[R14_SVC].I;
		reg[RN_CPSR].I = SPSR;
		if(saveState)
			reg[RN_SPSR].I = CPSR;
		else
			reg[RN_SPSR].I = reg[SPSR_SVC].I;
		break;
	case 0x17:
		reg[13].I = reg[R13_ABT].I;
		reg[14].I = reg[R14_ABT].I;
		reg[RN_CPSR].I = SPSR;
		if(saveState)
			reg[RN_SPSR].I = CPSR;
		else
			reg[RN_SPSR].I = reg[SPSR_ABT].I;
		break;
	case 0x1b:
		reg[13].I = reg[R13_UND].I;
		reg[14].I = reg[R14_UND].I;
		reg[RN_CPSR].I = SPSR;
		if(saveState)
			reg[RN_SPSR].I = CPSR;
		else
			reg[RN_SPSR].I = reg[SPSR_UND].I;
		break;
	default:
		// An illegal mode causes the processor to enter an unrecoverable state
		ERROR_LOG(AICA_ARM, "Unsupported ARM mode %02x", mode);
		Arm7Enabled = false;
		break;
	}
	armMode = mode;
	CPUUpdateFlags();
	CPUUpdateCPSR();
}

void CPUUpdateCPSR()
{
	reg_pair CPSR;

	CPSR.I = reg[RN_CPSR].I & 0x40;

	CPSR.PSR.NZCV = reg[RN_PSR_FLAGS].FLG.NZCV;

	if (!armFiqEnable)
		CPSR.I |= 0x40;
	if(!armIrqEnable)
		CPSR.I |= 0x80;

	CPSR.PSR.M = armMode;
	
	reg[RN_CPSR].I = CPSR.I;
}

static void CPUUpdateFlags()
{
	u32 CPSR = reg[RN_CPSR].I;

	reg[RN_PSR_FLAGS].FLG.NZCV = reg[RN_CPSR].PSR.NZCV;

	armIrqEnable = (CPSR & 0x80) ? false : true;
	armFiqEnable = (CPSR & 0x40) ? false : true;
	update_armintc();
}

static void CPUSoftwareInterrupt(int comment)
{
	u32 PC = reg[R15_ARM_NEXT].I+4;
	CPUSwitchMode(0x13, true);
	reg[14].I = PC;
	
	armIrqEnable = false;
	armNextPC = 0x08;
}

static void CPUUndefinedException()
{
	WARN_LOG(AICA_ARM, "arm7: CPUUndefinedException(). SOMETHING WENT WRONG");
	u32 PC = reg[R15_ARM_NEXT].I+4;
	CPUSwitchMode(0x1b, true);
	reg[14].I = PC;
	armIrqEnable = false;
	armNextPC = 0x04;
}

void reset()
{
	INFO_LOG(AICA_ARM, "AICA ARM Reset");
#if FEAT_AREC != DYNAREC_NONE
	recompiler::flush();
#endif
	aica_interr = false;
	aica_reg_L = 0;
	e68k_out = false;
	e68k_reg_L = 0;
	e68k_reg_M = 0;

	Arm7Enabled = false;
	// clean registers
	memset(&arm_Reg[0], 0, sizeof(arm_Reg));

	armMode = 0x13;

	reg[13].I = 0x03007F00;
	reg[15].I = 0x0000000;
	reg[RN_CPSR].I = 0x00000000;
	reg[R13_IRQ].I = 0x03007FA0;
	reg[R13_SVC].I = 0x03007FE0;
	armIrqEnable = true;      
	armFiqEnable = false;
	update_armintc();

	C_FLAG = V_FLAG = N_FLAG = Z_FLAG = false;

	// disable FIQ
	reg[RN_CPSR].I |= 0x40;

	CPUUpdateCPSR();

	armNextPC = reg[15].I;
	reg[15].I += 4;
}

void CPUFiq()
{
	u32 PC = reg[R15_ARM_NEXT].I+4;
	CPUSwitchMode(0x11, true);
	reg[14].I = PC;
	armIrqEnable = false;
	armFiqEnable = false;
	update_armintc();

	armNextPC = 0x1c;
}

/*
	--Seems like aica has 3 interrupt controllers actualy (damn lazy sega ..)
	The "normal" one (the one that exists on scsp) , one to emulate the 68k intc , and , 
	of course , the arm7 one

	The output of the sci* bits is input to the e68k , and the output of e68k is inputed into the FIQ
	pin on arm7
*/


void enable(bool enabled)
{
	if(!Arm7Enabled && enabled)
		reset();
	
	Arm7Enabled=enabled;
}

void update_armintc()
{
	reg[INTR_PEND].I=e68k_out && armFiqEnable;
}

#if FEAT_AREC != DYNAREC_NONE
//
// Used by ARM7 Recompiler
//
namespace recompiler {

//Emulate a single arm op, passed in opcode

void DYNACALL interpret(u32 opcode)
{
	u32 clockTicks = 0;

#define NO_OPCODE_READ
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
#include "arm-new.h"
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
#undef NO_OPCODE_READ

	reg[CYCL_CNT].I -= clockTicks;
}

template<u32 Pd>
void DYNACALL MSR_do(u32 v, u32 mask)
{
	if (Pd)
	{
		if(armMode > 0x10 && armMode < 0x1f) /* !=0x10 ?*/
		{
			u32 newValue = reg[RN_SPSR].I;
			if (mask & 1)
				newValue = (newValue & 0xFFFFFF00) | (v & 0x000000FF);
			if (mask & 2)
				newValue = (newValue & 0xFFFF00FF) | (v & 0x0000FF00);
			if (mask & 4)
				newValue = (newValue & 0xFF00FFFF) | (v & 0x00FF0000);
			if (mask & 8)
				newValue = (newValue & 0x00FFFFFF) | (v & 0xFF000000);
			reg[RN_SPSR].I = newValue;
		}
	}
	else
	{
		CPUUpdateCPSR();
	
		u32 newValue = reg[RN_CPSR].I;
		if(armMode > 0x10)
		{
			if (mask & 1)
				newValue = (newValue & 0xFFFFFF00) | (v & 0x000000FF);
			if (mask & 2)
				newValue = (newValue & 0xFFFF00FF) | (v & 0x0000FF00);
			if (mask & 4)
				newValue = (newValue & 0xFF00FFFF) | (v & 0x00FF0000);
		}
		if (mask & 8)
			newValue = (newValue & 0x00FFFFFF) | (v & 0xFF000000);
		newValue |= 0x10;
		if(armMode > 0x10)
		{
			CPUSwitchMode(newValue & 0x1f, false);
		}
		reg[RN_CPSR].I = newValue;
		CPUUpdateFlags();
	}
}
template void DYNACALL MSR_do<0>(u32 v, u32 mask);
template void DYNACALL MSR_do<1>(u32 v, u32 mask);

} // namespace recompiler
#endif	// FEAT_AREC != DYNAREC_NONE

} // namespace aica::arm
