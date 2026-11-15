#pragma once
#include "types.h"

namespace aica::sgc
{

void AICA_Sample();
void AICA_Samples(u32 samples);
/* the port: a channel's model, or every channel's, brought up to the sample
 * the AICA is at (sgc_if.cpp's ChannelEx::Sync); nothing without FLYCAST_APU */
void SyncChannel(u32 channel);
void SyncAllChannels();

void WriteChannelReg(u32 channel, u32 reg, int size);

void init();
void term();

union fp_22_10
{
	struct
	{
		u32 fp:10;
		u32 ip:22;
	};
	u32 full;
};
union fp_s_22_10
{
	struct
	{
		u32 fp:10;
		s32 ip:22;
	};
	s32 full;
};
union fp_20_12
{
	struct
	{
		u32 fp:12;
		u32 ip:20;
	};
	u32 full;
};

typedef s32 SampleType;

void ReadCommonReg(u32 reg, bool byte);
void serialize(Serializer& ctx);
void deserialize(Deserializer& ctx);
void vmuBeep(int on, int period);

} // namespace aica::sgc
