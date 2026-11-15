/* the port's stand-in for Flycast's audiostream.h: each mixed stereo sample
 * goes to the host's output, xbox/host/pc/sound.cpp */
#pragma once
#include "types.h"
void WriteSample(s16 right, s16 left);
