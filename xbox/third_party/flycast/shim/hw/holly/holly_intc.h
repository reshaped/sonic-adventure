/* the port's stand-in for Flycast's holly_intc.h: the AICA's interrupt to
 * the SH-4, which the port's sound library never takes */
#pragma once
#include "types.h"
enum HollyInterruptID { holly_SPU_IRQ = 0x0101 };
static inline void asic_RaiseInterrupt(HollyInterruptID) {}
static inline void asic_CancelInterrupt(HollyInterruptID) {}
