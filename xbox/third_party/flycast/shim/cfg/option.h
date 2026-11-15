/* the port's stand-in for Flycast's option.h: the options the AICA reads */
#pragma once
#include "types.h"
namespace config {
template<typename T> struct Value {
	T value;
	operator T() const { return value; }
};
extern Value<bool> DSPEnabled;
extern Value<bool> GGPOEnable;
extern Value<bool> VmuSound;
}
struct flycast_settings {
	struct { bool muteAudio; } aica;
	struct { bool fastForwardMode; } input;
	struct { bool isAtomiswave; } platform;
};
extern flycast_settings settings;
