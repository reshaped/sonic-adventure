/* the port's stand-in for Flycast's core/types.h: the integer types, the
 * logs and the checks that Flycast's AICA and ARM7 sources use, and no more.
 * the port's sound runs these sources in its host (xbox/host/pc/sound.cpp) */
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "build.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef float f32;

#define DYNACALL
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

/* the logs, which the port leaves silent (xbox/host/pc/sound.cpp) */
void flycast_log(const char *kind, const char *format, ...) __attribute__((format(printf, 2, 3)));
#define DEBUG_LOG(type, ...) ((void)0)
#define INFO_LOG(type, ...) flycast_log("info", __VA_ARGS__)
#define NOTICE_LOG(type, ...) flycast_log("notice", __VA_ARGS__)
#define WARN_LOG(type, ...) flycast_log("warn", __VA_ARGS__)
#define ERROR_LOG(type, ...) flycast_log("error", __VA_ARGS__)
#define verify(x) ((x) ? (void)0 : flycast_die(__FILE__, __LINE__, #x))
#define die(reason) flycast_die(__FILE__, __LINE__, reason)
[[noreturn]] void flycast_die(const char *file, int line, const char *what);

/* the save states, which the port doesn't take */
class Serializer;
class Deserializer;

/* the AICA's 2 MB of sound RAM */
#define ARAM_SIZE (2u * 1024 * 1024)
#define ARAM_MASK (ARAM_SIZE - 1)

constexpr size_t operator""_KB(unsigned long long x) { return 1024 * x; }
constexpr size_t operator""_MB(unsigned long long x) { return 1024 * 1024 * x; }

template<typename T>
T ReadMemArr(const u8 *array, u32 addr)
{
	return *(const T *)&array[addr & ~(sizeof(T) - 1)];
}

template<typename T>
void WriteMemArr(u8 *array, u32 addr, T data)
{
	*(T *)&array[addr & ~(sizeof(T) - 1)] = data;
}
