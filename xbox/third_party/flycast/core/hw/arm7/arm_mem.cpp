#include "arm_mem.h"
#include "hw/aica/aica_mem.h"

#ifdef FLYCAST_TRACE
extern "C" void flycast_trace(const char *kind, unsigned a, unsigned b);
#endif

namespace aica::arm
{

#define REG_L (0x2D00)
#define REG_M (0x2D04)

//Set to true when aica interrupt is pending
bool aica_interr;
u32 aica_reg_L;
//Set to true when the out of the intc is 1
bool e68k_out;
u32 e68k_reg_L;
u32 e68k_reg_M; //constant ?

void update_e68k()
{
	if (!e68k_out && aica_interr)
	{
		//Set the pending signal
		//Is L register held here too ?
		e68k_out = true;
		e68k_reg_L = aica_reg_L;

		update_armintc();
	}
}

void interruptChange(u32 bits,u32 L)
{
	aica_interr = bits != 0;
	if (aica_interr)
		aica_reg_L = L;
	update_e68k();
}

void e68k_AcceptInterrupt()
{
	e68k_out = false;
	update_e68k();
	update_armintc();
}

//Reg reads from arm side ..
#if defined(FLYCAST_AICA_ONLY) && defined(FLYCAST_PROFILE)
/* the port: the ARM7's reads of the AICA's registers, by 4-byte word, for
 * HOST_AICA_READS (xbox/host/pc/sound.cpp) */
unsigned profile_reads[0x8000 / 4];
#endif

template <typename T>
T readReg(u32 addr)
{
#if defined(FLYCAST_AICA_ONLY) && defined(FLYCAST_PROFILE)
	profile_reads[(addr & 0x7fff) / 4]++;
#endif
	addr &= 0x7FFF;
	if (addr == REG_L)
		return (T)e68k_reg_L;
	else if (addr == REG_M)
		return (T)e68k_reg_M;	//shouldn't really happen
	else if (sizeof(T) == 4)
		return readRegInternal<u16>(addr);
	else
		return readRegInternal<T>(addr);
}

#if defined(FLYCAST_AICA_ONLY) && defined(FLYCAST_PROFILE)
unsigned profile_writes[0x8000 / 4];
#endif

template <typename T>
void writeReg(u32 addr, T data)
{
#if defined(FLYCAST_AICA_ONLY) && defined(FLYCAST_PROFILE)
	profile_writes[(addr & 0x7fff) / 4]++;
#endif
	addr &= 0x7FFF;
#ifdef FLYCAST_TRACE
	/* the port: each register write of the ARM7's, for sndreplay's trace */
	flycast_trace(sizeof(T) == 1 ? "w8" : sizeof(T) == 2 ? "w16" : "w32", addr, (unsigned)data);
#endif
	if (addr == REG_L)
	{
		return; // Shouldn't really happen (read only)
	}
	else if (addr == REG_M)
	{
		//accept interrupts
		if (data & 1)
			e68k_AcceptInterrupt();
	}
	else
	{
		if (sizeof(T) == 4)
			writeRegInternal(addr, (u16)data);
		else
			writeRegInternal(addr, data);
	}
}

//00000000~007FFFFF @DRAM_AREA* 
//00800000~008027FF @CHANNEL_DATA 
//00802800~00802FFF @COMMON_DATA 
//00803000~00807FFF @DSP_DATA 

template u8 readReg<u8>(u32 adr);
template u16 readReg<u16>(u32 adr);
template u32 readReg<u32>(u32 adr);

template void writeReg<>(u32 adr, u8 data);
template void writeReg<>(u32 adr, u16 data);
template void writeReg<>(u32 adr, u32 data);

} // namespace aica::arm
