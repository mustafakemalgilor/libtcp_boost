#pragma once

#	include <string>
#	include <cassert>

#define TEST_MODE

#	define ASSERT assert
#define TRACE(x) OutputDebugStringA(x)

typedef signed __int64 int64;
typedef signed __int32 int32;
typedef signed __int16 int16;
typedef signed __int8 int8;
typedef unsigned __int64 uint64;
typedef unsigned __int32 uint32;
typedef unsigned __int16 uint16;
typedef unsigned __int8 uint8;

#	define INLINE __forceinline
#	define swap16(p) p
#	define swap32(p) p
#	define swap64(p) p
#	define swapfloat(p) p
#	define swapdouble(p) p


