#pragma once

#ifdef _WIN64
	typedef __int64 ssize_t;
	#define SSIZE_MAX _I64_MAX
#else
	typedef __int32 ssize_t;
	#define SSIZE_MAX _I32_MAX
#endif

#define _S(x) #x
#define _ST(x) _S(x)

#define REVISION 1
#define VERSION "r" _ST(REVISION_STR)

#define CPUSTRING "x86"

#if _DEBUG
	#define BUILDSTRING "Debug"
#else
	#define BUILDSTRING "Release"
#endif

#define Q2GAME "baseq2"
#define BASEGAME "genx"
#define DEFGAME BASEGAME
#define VID_REF "gl"
#define VID_GEOMETRY "640x480"

#define USE_PNG 1
#define USE_SERVER 1
#define USE_CLIENT 1
#define USE_OPENAL 1
#define USE_UI 1
#define USE_LIGHTSTYLES 1
#define USE_DLIGHTS 1
#define USE_MAPCHECKSUM 1
#define REF_GL 1
#define USE_REF 1
#define USE_ZLIB 1
#define USE_SYSCON 1
#define USE_FPS 1
#define USE_DINPUT 1
#define USE_TGA 1
