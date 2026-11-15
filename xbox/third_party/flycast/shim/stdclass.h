/* the port's stand-in for Flycast's core/stdclass.h: RamRegion alone, over
 * memory the host maps, the AICA's sound RAM */
#pragma once
#include "types.h"

class RamRegion
{
	u8 *data = nullptr;
	size_t size = 0;

public:
	void setRegion(u8 *data, size_t size)
	{
		this->data = data;
		this->size = size;
	}
	void zero() { std::memset(data, 0, size); }
	u8& operator [](size_t i) { return data[i]; }
};

/* Flycast runs a function at load through a static OnLoad. under
 * FLYCAST_DEFER_ONLOAD, as the Xbox host builds it, the functions wait for
 * flycast_run_onloads(): their tables use the FPU, which a static
 * constructor runs too early to use, before nxdk's startup reaches main */
#ifdef FLYCAST_DEFER_ONLOAD
void flycast_defer_onload(void (*f)());
void flycast_run_onloads();
struct OnLoad
{
	explicit OnLoad(void (*f)()) { flycast_defer_onload(f); }
};
#else
struct OnLoad
{
	explicit OnLoad(void (*f)()) { f(); }
};
#endif
