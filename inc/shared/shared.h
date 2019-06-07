/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef SHARED_H
#define SHARED_H

//
// shared.h -- included first by ALL program modules
//

#include "config.h"

#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <limits.h>
#include <time.h>

#if HAVE_ENDIAN_H
	#include <endian.h>
#endif

#include "shared/platform.h"

#define q_countof(a)        (sizeof(a) / sizeof(a[0]))

typedef unsigned char byte;

#define OPAQUE_TYPE \
	struct { int unused; } *

typedef OPAQUE_TYPE q_soundhandle;
typedef OPAQUE_TYPE q_modelhandle;
typedef OPAQUE_TYPE q_imagehandle;

typedef OPAQUE_TYPE qhandle_t;

#define MODEL_HANDLE_PLAYER ((q_modelhandle)(USHRT_MAX - 1))

// Paril
typedef enum
{
	GAME_NONE,

	GAME_Q2,
	GAME_Q1,
	GAME_DOOM,
	GAME_DUKE,

	GAME_TOTAL
} gametype_t;

extern const char *gameNames[];

// Item enums
typedef enum
{
	// Quake 2 stuff
	ITI_NULL,
	ITI_FIRST,

	ITI_JACKET_ARMOR = ITI_FIRST,
	ITI_COMBAT_ARMOR,
	ITI_BODY_ARMOR,

	ITI_ARMOR_SHARD,
	ITI_POWER_SCREEN,
	ITI_POWER_SHIELD,

	ITI_WEAPONS_START,
	ITI_WEAPON_1 = ITI_WEAPONS_START,
	ITI_WEAPON_2,
	ITI_WEAPON_3,
	ITI_WEAPON_4,
	ITI_WEAPON_5,
	ITI_WEAPON_6,
	ITI_WEAPON_7,
	ITI_WEAPON_8,
	ITI_WEAPON_9,
	ITI_WEAPON_0,
	ITI_WEAPON_G,
	ITI_WEAPONS_END = ITI_WEAPON_G,

	ITI_AMMO,

	ITI_STIMPACK,
	ITI_MEDIUM_HEALTH,
	ITI_LARGE_HEALTH,
	ITI_MEGA_HEALTH,

	ITI_QUAD_DAMAGE,
	ITI_INVULNERABILITY,
	ITI_ENVIRONMENT_SUIT,
	ITI_SILENCER,
	ITI_REBREATHER,
	ITI_ANCIENT_HEAD,
	ITI_ADRENALINE,
	ITI_BANDOLIER,
	ITI_AMMO_PACK,

	ITI_DATA_CD,
	ITI_POWER_CUBE,
	ITI_PYRAMID_KEY,
	ITI_DATA_SPINNER,
	ITI_SECURITY_PASS,
	ITI_BLUE_KEY,
	ITI_RED_KEY,
	ITI_COMMANDERS_HEAD,
	ITI_AIRSTRIKE_MARKER,

	ITI_TOTAL,
	ITI_LAST = ITI_TOTAL - 1,

	// Quake 2
	ITI_Q2_BLASTER = ITI_WEAPON_1,
	ITI_Q2_SHOTGUN = ITI_WEAPON_2,
	ITI_Q2_SUPER_SHOTGUN = ITI_WEAPON_3,
	ITI_Q2_MACHINEGUN = ITI_WEAPON_4,
	ITI_Q2_CHAINGUN = ITI_WEAPON_5,
	ITI_Q2_GRENADE_LAUNCHER = ITI_WEAPON_6,
	ITI_Q2_ROCKET_LAUNCHER = ITI_WEAPON_7,
	ITI_Q2_HYPERBLASTER = ITI_WEAPON_8,
	ITI_Q2_RAILGUN = ITI_WEAPON_9,
	ITI_Q2_BFG10K = ITI_WEAPON_0,
	ITI_Q2_GRENADES = ITI_WEAPON_G,

	// Quake 1
	ITI_Q1_GREEN_ARMOR = ITI_JACKET_ARMOR,
	ITI_Q1_YELLOW_ARMOR = ITI_COMBAT_ARMOR,
	ITI_Q1_RED_ARMOR = ITI_BODY_ARMOR,
	ITI_Q1_INVISIBILITY = ITI_REBREATHER,

	ITI_Q1_AXE = ITI_WEAPON_1,
	ITI_Q1_SHOTGUN = ITI_WEAPON_2,
	ITI_Q1_SUPER_SHOTGUN = ITI_WEAPON_3,
	ITI_Q1_NAILGUN = ITI_WEAPON_4,
	ITI_Q1_SUPER_NAILGUN = ITI_WEAPON_5,
	ITI_Q1_GRENADE_LAUNCHER = ITI_WEAPON_6,
	ITI_Q1_ROCKET_LAUNCHER = ITI_WEAPON_7,
	ITI_Q1_THUNDERBOLT = ITI_WEAPON_8,

	// Doom
	ITI_DOOM_ARMOR = ITI_JACKET_ARMOR,
	ITI_DOOM_MEGA_ARMOR = ITI_COMBAT_ARMOR,

	ITI_DOOM_MEGA_SPHERE = ITI_MEGA_HEALTH,
	ITI_DOOM_SUPER_SPHERE = ITI_ADRENALINE,
	ITI_DOOM_INVISIBILITY = ITI_REBREATHER,

	ITI_DOOM_FIST = ITI_WEAPON_1,
	ITI_DOOM_PISTOL = ITI_WEAPON_2,
	ITI_DOOM_SHOTGUN = ITI_WEAPON_3,
	ITI_DOOM_CHAINGUN = ITI_WEAPON_4,
	ITI_DOOM_ROCKET_LAUNCHER = ITI_WEAPON_5,
	ITI_DOOM_PLASMA_GUN = ITI_WEAPON_6,
	ITI_DOOM_BFG = ITI_WEAPON_7,
	ITI_DOOM_SUPER_SHOTGUN = ITI_WEAPON_8,
	ITI_DOOM_CHAINSAW = ITI_WEAPON_0,

	// Duke
	ITI_DUKE_FOOT = ITI_WEAPON_1,
	ITI_DUKE_PISTOL = ITI_WEAPON_2,
	ITI_DUKE_SHOTGUN = ITI_WEAPON_3,
	ITI_DUKE_CANNON = ITI_WEAPON_4,
	ITI_DUKE_RPG = ITI_WEAPON_5,
	ITI_DUKE_PIPEBOMBS = ITI_WEAPON_6,
	//ITI_DUKE_SHRINKER = ITI_WEAPON_7,
	ITI_DUKE_DEVASTATOR = ITI_WEAPON_8,
	ITI_DUKE_TRIPWIRES = ITI_WEAPON_9,
	ITI_DUKE_FREEZER = ITI_WEAPON_0,

	ITI_DUKE_ARMOR = ITI_JACKET_ARMOR,

	// a special value used for stuff
	ITI_UNUSED = -1
} itemid_e;

#ifndef NULL
	#define NULL ((void *)0)
#endif

// angle indexes
#define PITCH               0       // up / down
#define YAW                 1       // left / right
#define ROLL                2       // fall over

#define MAX_STRING_CHARS    1024    // max length of a string passed to Cmd_TokenizeString
#define MAX_STRING_TOKENS   256     // max tokens resulting from Cmd_TokenizeString
#define MAX_TOKEN_CHARS     1024    // max length of an individual token
#define MAX_NET_STRING      2048    // max length of a string used in network protocol

#define MAX_QPATH           64      // max length of a quake game pathname
#define MAX_OSPATH          256     // max length of a filesystem pathname

//
// per-level limits
//
#define MAX_CLIENTS         256     // absolute limit
#define MAX_EDICTS          1024    // must change protocol to increase more
#define MAX_LIGHTSTYLES     256
// Generations
#define MAX_PRECACHE        1024
#define MAX_PRECACHE_BITSET (MAX_PRECACHE / 3) + 1
#define MAX_GENERAL         (MAX_CLIENTS * 2) // general config strings

#define MAX_CLIENT_NAME     16

typedef enum
{
	ERR_FATAL,          // exit the entire game with a popup window
	ERR_DROP,           // print to console and disconnect from game
	ERR_DISCONNECT,     // like drop, but not an error
	ERR_RECONNECT       // make server broadcast 'reconnect' message
} error_type_t;

typedef enum
{
	PRINT_ALL,          // general messages
	PRINT_TALK,         // print in green color
	PRINT_DEVELOPER,    // only print when "developer 1"
	PRINT_WARNING,      // print in yellow color
	PRINT_ERROR,        // print in red color
	PRINT_NOTICE        // print in cyan color
} print_type_t;

void    Com_LPrintf(print_type_t type, const char *fmt, ...)
q_printf(2, 3);
void    Com_Error(error_type_t code, const char *fmt, ...)
q_noreturn q_printf(2, 3);

#define Com_Printf(...) Com_LPrintf(PRINT_ALL, __VA_ARGS__)
#define Com_WPrintf(...) Com_LPrintf(PRINT_WARNING, __VA_ARGS__)
#define Com_EPrintf(...) Com_LPrintf(PRINT_ERROR, __VA_ARGS__)

// game print flags
#define PRINT_LOW           0       // pickup messages
#define PRINT_MEDIUM        1       // death messages
#define PRINT_HIGH          2       // critical messages
#define PRINT_CHAT          3       // chat messages    

// destination class for gi.multicast()
typedef enum
{
	MULTICAST_ALL,
	MULTICAST_PHS,
	MULTICAST_PVS,
	MULTICAST_ALL_R,
	MULTICAST_PHS_R,
	MULTICAST_PVS_R
} multicast_t;

/*
==============================================================

MATHLIB

==============================================================
*/

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];

typedef float mat4_t[16];

typedef union
{
	uint32_t u32;
	uint8_t u8[4];
} color_t;

static inline color_t ColorFromRGBA(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a)
{
	color_t c = { 0 };
	c.u8[0] = r;
	c.u8[1] = g;
	c.u8[2] = b;
	c.u8[3] = a;
	return c;
}

typedef int fixed4_t;
typedef int fixed8_t;
typedef int fixed16_t;

#ifndef M_PI
	#define M_PI        3.14159265358979323846  // matches value in gcc v2 math.h
#endif

struct cplane_s;

extern vec3_t vec3_origin;

typedef struct vrect_s
{
	int             x, y, width, height;
} vrect_t;

#define DEG2RAD(a)      ((a) * (M_PI / 180))
#define RAD2DEG(a)      ((a) * (180 / M_PI))

#define ALIGN(x, a)     (((x) + (a) - 1) & ~((a) - 1))

static inline float LerpFloat(const float a, const float b, const float frac)
{
	return a + (b - a) * frac;
}

#define DotProduct(x,y)         ((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define CrossProduct(v1,v2,cross) \
	((cross)[0]=(v1)[1]*(v2)[2]-(v1)[2]*(v2)[1], \
		(cross)[1]=(v1)[2]*(v2)[0]-(v1)[0]*(v2)[2], \
		(cross)[2]=(v1)[0]*(v2)[1]-(v1)[1]*(v2)[0])
#define VectorSubtract(a,b,c) \
	((c)[0]=(a)[0]-(b)[0], \
		(c)[1]=(a)[1]-(b)[1], \
		(c)[2]=(a)[2]-(b)[2])
#define VectorAdd(a,b,c) \
	((c)[0]=(a)[0]+(b)[0], \
		(c)[1]=(a)[1]+(b)[1], \
		(c)[2]=(a)[2]+(b)[2])
#define VectorAdd3(a,b,c,d) \
	((d)[0]=(a)[0]+(b)[0]+(c)[0], \
		(d)[1]=(a)[1]+(b)[1]+(c)[1], \
		(d)[2]=(a)[2]+(b)[2]+(c)[2])
#define VectorCopy(a,b)     ((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define VectorClear(a)      ((a)[0]=(a)[1]=(a)[2]=0)
#define VectorNegate(a,b)   ((b)[0]=-(a)[0],(b)[1]=-(a)[1],(b)[2]=-(a)[2])
#define VectorInverse(a)    ((a)[0]=-(a)[0],(a)[1]=-(a)[1],(a)[2]=-(a)[2])
#define VectorSet(v, x, y, z)   ((v)[0]=(x),(v)[1]=(y),(v)[2]=(z))
#define VectorAvg(a,b,c) \
	((c)[0]=((a)[0]+(b)[0])*0.5f, \
		(c)[1]=((a)[1]+(b)[1])*0.5f, \
		(c)[2]=((a)[2]+(b)[2])*0.5f)
#define VectorMA(a,b,c,d) \
	((d)[0]=(a)[0]+(b)*(c)[0], \
		(d)[1]=(a)[1]+(b)*(c)[1], \
		(d)[2]=(a)[2]+(b)*(c)[2])
#define VectorVectorMA(a,b,c,d) \
	((d)[0]=(a)[0]+(b)[0]*(c)[0], \
		(d)[1]=(a)[1]+(b)[1]*(c)[1], \
		(d)[2]=(a)[2]+(b)[2]*(c)[2])
#define VectorEmpty(v) ((v)[0]==0&&(v)[1]==0&&(v)[2]==0)
#define VectorCompare(v1,v2)    ((v1)[0]==(v2)[0]&&(v1)[1]==(v2)[1]&&(v1)[2]==(v2)[2])
#define VectorLength(v)     (sqrtf(DotProduct((v),(v))))
#define VectorLengthSquared(v)      (DotProduct((v),(v)))
#define VectorScale(in,scale,out) \
	((out)[0]=(in)[0]*(scale), \
		(out)[1]=(in)[1]*(scale), \
		(out)[2]=(in)[2]*(scale))
#define VectorVectorScale(in,scale,out) \
	((out)[0]=(in)[0]*(scale)[0], \
		(out)[1]=(in)[1]*(scale)[1], \
		(out)[2]=(in)[2]*(scale)[2])
#define DistanceSquared(v1,v2) \
	(((v1)[0]-(v2)[0])*((v1)[0]-(v2)[0])+ \
		((v1)[1]-(v2)[1])*((v1)[1]-(v2)[1])+ \
		((v1)[2]-(v2)[2])*((v1)[2]-(v2)[2]))
#define Distance(v1,v2) (sqrtf(DistanceSquared(v1,v2)))
#define ManhattanDistance(v1,v2) (fabsf(v2[0] - v1[0]) + fabsf(v2[1] - v1[1]) + fabsf(v2[2] - v1[2]))
#define LerpAngles(a,b,c,d) \
	((d)[0]=LerpAngle((a)[0],(b)[0],c), \
		(d)[1]=LerpAngle((a)[1],(b)[1],c), \
		(d)[2]=LerpAngle((a)[2],(b)[2],c))
#define LerpVector(a,b,c,d) \
	((d)[0]=(a)[0]+(c)*((b)[0]-(a)[0]), \
		(d)[1]=(a)[1]+(c)*((b)[1]-(a)[1]), \
		(d)[2]=(a)[2]+(c)*((b)[2]-(a)[2]))
#define LerpVector2(a,b,c,d,e) \
	((e)[0]=(a)[0]*(c)+(b)[0]*(d), \
		(e)[1]=(a)[1]*(c)+(b)[1]*(d), \
		(e)[2]=(a)[2]*(c)+(b)[2]*(d))
#define PlaneDiff(v,p)   (DotProduct(v,(p)->normal)-(p)->dist)

#define Vector4Subtract(a,b,c)  ((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2],(c)[3]=(a)[3]-(b)[3])
#define Vector4Add(a,b,c)       ((c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1],(c)[2]=(a)[2]+(b)[2],(c)[3]=(a)[3]+(b)[3])
#define Vector4Copy(a,b)        ((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3])
#define Vector4Clear(a)         ((a)[0]=(a)[1]=(a)[2]=(a)[3]=0)
#define Vector4Negate(a,b)      ((b)[0]=-(a)[0],(b)[1]=-(a)[1],(b)[2]=-(a)[2],(b)[3]=-(a)[3])
#define Vector4Set(v, a, b, c, d)   ((v)[0]=(a),(v)[1]=(b),(v)[2]=(c),(v)[3]=(d))
#define Vector4Compare(v1,v2)    ((v1)[0]==(v2)[0]&&(v1)[1]==(v2)[1]&&(v1)[2]==(v2)[2]&&(v1)[3]==(v2)[3])

void AngleVectors(vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
vec_t VectorNormalize(vec3_t v);        // returns vector length
vec_t VectorNormalize2(vec3_t v, vec3_t out);
void ClearBounds(vec3_t mins, vec3_t maxs);
void AddPointToBounds(const vec3_t v, vec3_t mins, vec3_t maxs);
vec_t RadiusFromBounds(const vec3_t mins, const vec3_t maxs);
void UnionBounds(vec3_t a[2], vec3_t b[2], vec3_t c[2]);

static inline void AnglesToAxis(vec3_t angles, vec3_t axis[3])
{
	AngleVectors(angles, axis[0], axis[1], axis[2]);
	VectorInverse(axis[1]);
}

static inline void TransposeAxis(vec3_t axis[3])
{
	vec_t temp;
	temp = axis[0][1];
	axis[0][1] = axis[1][0];
	axis[1][0] = temp;
	temp = axis[0][2];
	axis[0][2] = axis[2][0];
	axis[2][0] = temp;
	temp = axis[1][2];
	axis[1][2] = axis[2][1];
	axis[2][1] = temp;
}

static inline void RotatePoint(vec3_t point, vec3_t axis[3])
{
	vec3_t temp;
	VectorCopy(point, temp);
	point[0] = DotProduct(temp, axis[0]);
	point[1] = DotProduct(temp, axis[1]);
	point[2] = DotProduct(temp, axis[2]);
}

static inline unsigned npot32(unsigned k)
{
	if (k == 0)
		return 1;

	k--;
	k = k | (k >> 1);
	k = k | (k >> 2);
	k = k | (k >> 4);
	k = k | (k >> 8);
	k = k | (k >> 16);
	return k + 1;
}

static inline float LerpAngle(float a2, float a1, float frac)
{
	if (a1 - a2 > 180)
		a1 -= 360;

	if (a1 - a2 < -180)
		a1 += 360;

	return a2 + frac * (a1 - a2);
}

static inline float anglemod(float a)
{
	a = (360.0f / 65536) * ((int)(a * (65536 / 360.0f)) & 65535);
	return a;
}

static inline int Q_align(int value, int align)
{
	int mod = value % align;
	return mod ? value + align - mod : value;
}

static inline int Q_gcd(int a, int b)
{
	while (b != 0)
	{
		int t = b;
		b = a % b;
		a = t;
	}

	return a;
}

void Q_srand(uint32_t seed);
uint32_t Q_rand(void);
uint32_t Q_rand_uniform(uint32_t n);

#define clamp(a,b,c)    ((a)<(b)?(a)=(b):(a)>(c)?(a)=(c):(a))
#define cclamp(a,b,c)   ((b)>(c)?clamp(a,c,b):clamp(a,b,c))

#ifndef max
	#define max(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef min
	#define min(a,b) ((a)<(b)?(a):(b))
#endif

static inline float frand(void)
{
	return (int32_t)Q_rand() * 0x1p-32f + 0.5f;
}

static inline float crand(void)
{
	return (int32_t)Q_rand() * 0x1p-31f;
}

static inline int Q_rint(float x)
{
	return x < 0 ? (int)(x - 0.5f) : (int)(x + 0.5f);
}

static inline bool Q_IsBitSet(const byte *data, const uint32_t bit)
{
	return (data[bit >> 3] & (1 << (bit & 7))) != 0;
}

static inline void Q_SetBit(byte *data, const uint32_t bit)
{
	data[bit >> 3] |= (1 << (bit & 7));
}

static inline void Q_ClearBit(byte *data, const uint32_t bit)
{
	data[bit >> 3] &= ~(1 << (bit & 7));
}

// Generations
typedef enum
{
	PRECAHCE_NONE,

	PRECACHE_MODEL,
	PRECACHE_SOUND,
	PRECACHE_PIC
} precache_type_e;

static inline precache_type_e Q_GetPrecacheBitsetType(const byte *bitset, const uint32_t index)
{
	const uint32_t bit_start = index * 3;
	byte value = 0;

	for (int i = 0; i < 3; ++i)
	{
		if (Q_IsBitSet(bitset, bit_start + i))
			value |= 1 << i;
	}

	return (precache_type_e)value;
}

static inline void Q_SetPrecacheBitsetType(byte *bitset, const uint32_t index, const precache_type_e value)
{
	const uint32_t bit_start = index * 3;

	for (int i = 0; i < 3; ++i)
	{
		if ((int)value & 1 << i)
			Q_SetBit(bitset, bit_start + i);
		else
			Q_ClearBit(bitset, bit_start + i);
	}
}

//=============================================

static inline bool Q_isupper(char c)
{
	return c >= 'A' && c <= 'Z';
}

static inline bool Q_islower(char c)
{
	return c >= 'a' && c <= 'z';
}

static inline bool Q_isdigit(char c)
{
	return c >= '0' && c <= '9';
}

static inline bool Q_isalpha(char c)
{
	return Q_isupper(c) || Q_islower(c);
}

static inline bool Q_isalnum(char c)
{
	return Q_isalpha(c) || Q_isdigit(c);
}

static inline bool Q_isprint(char c)
{
	return c >= 32 && c < 127;
}

static inline bool Q_isgraph(char c)
{
	return c > 32 && c < 127;
}

static inline bool Q_isspace(char c)
{
	return	c == ' ' || c == '\f' || c == '\n' ||
			c == '\r' || c == '\t' || c == '\v';
}

// tests if specified character is valid quake path character
static inline bool Q_ispath(char c)
{
	return Q_isalnum(c) || c == '_' || c == '-';
}

// tests if specified character has special meaning to quake console
static inline bool Q_isspecial(char c)
{
	return c == '\r' || c == '\n' || c == 127;
}

static inline int Q_tolower(int c)
{
	if (Q_isupper(c))
		c += ('a' - 'A');

	return c;
}

static inline int Q_toupper(int c)
{
	if (Q_islower(c))
		c -= ('a' - 'A');

	return c;
}

static inline char *Q_strlwr(char *s)
{
	char *p = s;

	while (*p)
	{
		*p = Q_tolower(*p);
		p++;
	}

	return s;
}

static inline char *Q_strupr(char *s)
{
	char *p = s;

	while (*p)
	{
		*p = Q_toupper(*p);
		p++;
	}

	return s;
}

static inline int Q_charhex(int c)
{
	if (c >= 'A' && c <= 'F')
		return 10 + (c - 'A');

	if (c >= 'a' && c <= 'f')
		return 10 + (c - 'a');

	if (c >= '0' && c <= '9')
		return c - '0';

	return -1;
}

// converts quake char to ASCII equivalent
static inline int Q_charascii(int c)
{
	if (Q_isspace(c))
	{
		// white-space chars are output as-is
		return c;
	}

	c &= 127; // strip high bits

	if (Q_isprint(c))
		return c;

	switch (c)
	{
		// handle bold brackets
		case 16:
			return '[';

		case 17:
			return ']';
	}

	return '.'; // don't output control chars, etc
}

// portable case insensitive compare
int Q_strcasecmp(const char *s1, const char *s2);
int Q_strncasecmp(const char *s1, const char *s2, size_t n);
char *Q_strcasestr(const char *s1, const char *s2);

#define Q_stricmp   Q_strcasecmp
#define Q_stricmpn  Q_strncasecmp
#define Q_stristr   Q_strcasestr

char *Q_strchrnul(const char *s, int c);
void *Q_memccpy(void *dst, const void *src, int c, size_t size);
void Q_setenv(const char *name, const char *value);

char *COM_SkipPath(const char *pathname);
size_t COM_StripExtension(char *out, const char *in, size_t size);
size_t COM_DefaultExtension(char *path, const char *ext, size_t size);
char *COM_FileExtension(const char *in);

#define COM_CompareExtension(in, ext) \
	Q_strcasecmp(COM_FileExtension(in), ext)

bool COM_IsFloat(const char *s);
bool COM_IsUint(const char *s);
bool COM_IsPath(const char *s);
bool COM_IsWhite(const char *s);

char *COM_Parse(const char **data_p);
// data is an in/out parm, returns a parsed out token
size_t COM_Compress(char *data);

int SortStrcmp(const void *p1, const void *p2);
int SortStricmp(const void *p1, const void *p2);

size_t COM_strclr(char *s);
char *COM_StripQuotes(char *s);

// buffer safe operations
size_t Q_strlcpy(char *dst, const char *src, size_t size);
size_t Q_strlcat(char *dst, const char *src, size_t size);

size_t Q_concat(char *dest, size_t size, ...) q_sentinel;

size_t Q_vsnprintf(char *dest, size_t size, const char *fmt, va_list argptr);
size_t Q_vscnprintf(char *dest, size_t size, const char *fmt, va_list argptr);
size_t Q_snprintf(char *dest, size_t size, const char *fmt, ...) q_printf(3, 4);
size_t Q_scnprintf(char *dest, size_t size, const char *fmt, ...) q_printf(3, 4);

char    *va(const char *format, ...) q_printf(1, 2);

//=============================================

static inline uint16_t ShortSwap(const uint16_t s)
{
	return (s >> 8) | (s << 8);
}

static inline uint32_t LongSwap(const uint32_t sl)
{
	uint32_t l = ((sl >> 8) & 0x00ff00ff) | ((sl << 8) & 0xff00ff00);
	l = (l >> 16) | (l << 16);
	return l;
}

static inline float FloatSwap(float f)
{
	union
	{
		float f;
		uint32_t l;
	} dat1, dat2;
	dat1.f = f;
	dat2.l = LongSwap(dat1.l);
	return dat2.f;
}

static inline uint16_t ShortNoSwap(const uint16_t s)
{
	return s;
}

static inline uint32_t LongNoSwap(const uint32_t s)
{
	return s;
}

static inline float FloatNoSwap(const float s)
{
	return s;
}

static inline uint32_t MakeRawLongLittle(const byte b1, const byte b2, const byte b3, const byte b4)
{
	return (((uint32_t)b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}

static inline uint16_t MakeRawShortLittle(const byte b1, const byte b2)
{
	return ((b2 << 8) | b1);
}

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define BigShort    ShortSwap
#define BigLong     LongSwap
#define BigFloat    FloatSwap
#define LittleShort    ShortNoSwap
#define LittleLong     LongNoSwap
#define LittleFloat    FloatNoSwap
#define MakeRawLong MakeRawLongLittle
#define MakeRawShort MakeRawShortLittle
#elif __BYTE_ORDER == __BIG_ENDIAN
#define BigShort     ShortNoSwap
#define BigLong      LongNoSwap
#define BigFloat     FloatNoSwap
#define LittleShort ShortSwap
#define LittleLong  LongSwap
#define LittleFloat FloatSwap
static inline uint32_t MakeRawLong(const byte b1, const byte b2, const byte b3, const byte b4)
{
	return MakeRawLongLittle(b4, b3, b2, b1);
}
static inline uint16_t MakeRawShort(const byte b1, const byte b2)
{
	return MakeRawShortLittle(b2, b1);
}
#else
#error Unknown byte order
#endif

static inline uint32_t LittleLongMem(const byte *p)
{
	return ((uint32_t)p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
}

static inline uint16_t LittleShortMem(const byte *p)
{
	return (p[1] << 8) | p[0];
}

static inline uint32_t RawLongMem(const byte *p)
{
	return MakeRawLong(p[0], p[1], p[2], p[3]);
}

static inline void LittleVector(const vec3_t in, vec3_t out)
{
	for (int i = 0; i < 3; i++)
		out[i] = LittleFloat(in[i]);
}

#define MakeColor	MakeRawLong

//=============================================

//
// key / value info strings
//
#define MAX_INFO_KEY        64
#define MAX_INFO_VALUE      64
#define MAX_INFO_STRING     512

char	    *Info_ValueForKey(const char *s, const char *key);
void	    Info_RemoveKey(char *s, const char *key);
bool		Info_SetValueForKey(char *s, const char *key, const char *value);
bool		Info_Validate(const char *s);
size_t		Info_SubValidate(const char *s);
void		Info_NextPair(const char **string, char *key, char *value);
void		Info_Print(const char *infostring);

/*
==========================================================

CVARS (console variables)

==========================================================
*/

#ifndef CVAR
#define CVAR

#define CVAR_ARCHIVE    1   // set to cause it to be saved to vars.rc
#define CVAR_USERINFO   2   // added to userinfo  when changed
#define CVAR_SERVERINFO 4   // added to serverinfo when changed
#define CVAR_NOSET      8   // don't allow change from console at all,
// but can be set from the command line
#define CVAR_LATCH      16  // save changes until server restart

struct cvar_s;
struct genctx_s;

typedef void (*xchanged_t)(struct cvar_s *);
typedef void (*xgenerator_t)(struct genctx_s *);

// nothing outside the cvar.*() functions should modify these fields!
typedef struct cvar_s
{
	char        *name;
	char        *string;
	char        *latched_string;    // for CVAR_LATCH vars
	int         flags;
	bool		modified;   // set each time the cvar is changed
	float       value;
	struct cvar_s *next;

	// ------ new stuff ------
	int         integer;
	char        *default_string;
	xchanged_t      changed;
	xgenerator_t    generator;
	struct cvar_s   *hashNext;
} cvar_t;

#endif      // CVAR

/*
==============================================================

COLLISION DETECTION

==============================================================
*/

// lower bits are stronger, and will eat weaker brushes completely
#define CONTENTS_SOLID          1       // an eye is never valid in a solid
#define CONTENTS_WINDOW         2       // translucent, but not watery
#define CONTENTS_AUX            4
#define CONTENTS_LAVA           8
#define CONTENTS_SLIME          16
#define CONTENTS_WATER          32
#define CONTENTS_MIST           64
#define LAST_VISIBLE_CONTENTS   64

// remaining contents are non-visible, and don't eat brushes

#define CONTENTS_AREAPORTAL     0x8000

#define CONTENTS_PLAYERCLIP     0x10000
#define CONTENTS_MONSTERCLIP    0x20000

// currents can be added to any other contents, and may be mixed
#define CONTENTS_CURRENT_0      0x40000
#define CONTENTS_CURRENT_90     0x80000
#define CONTENTS_CURRENT_180    0x100000
#define CONTENTS_CURRENT_270    0x200000
#define CONTENTS_CURRENT_UP     0x400000
#define CONTENTS_CURRENT_DOWN   0x800000

#define CONTENTS_ORIGIN         0x1000000   // removed before bsping an entity

#define CONTENTS_MONSTER        0x2000000   // should never be on a brush, only in game
#define CONTENTS_DEADMONSTER    0x4000000
#define CONTENTS_DETAIL         0x8000000   // brushes to be added after vis leafs
#define CONTENTS_TRANSLUCENT    0x10000000  // auto set if any surface has trans
#define CONTENTS_LADDER         0x20000000
// Generations
#define CONTENTS_BFG			0x40000000
#define CONTENTS_BULLETS		0x80000000



#define SURF_LIGHT      0x1     // value will hold the light strength

#define SURF_SLICK      0x2     // effects game physics

#define SURF_SKY        0x4     // don't draw, but add to skybox
#define SURF_WARP       0x8     // turbulent water warp
#define SURF_TRANS33    0x10
#define SURF_TRANS66    0x20
#define SURF_FLOWING    0x40    // scroll towards angle
#define SURF_NODRAW     0x80    // don't bother referencing the texture

#define SURF_ALPHATEST  0x02000000  // used by kmquake2



// content masks
#define MASK_ALL                (-1)
#define MASK_SOLID              (CONTENTS_SOLID|CONTENTS_WINDOW)
#define MASK_PLAYERSOLID        (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define MASK_DEADSOLID          (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW)
#define MASK_MONSTERSOLID       (CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define MASK_WATER              (CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define MASK_OPAQUE             (CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define MASK_SHOT               (CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEADMONSTER)
#define MASK_CURRENT            (CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN)
// Generations
#define MASK_BULLET				(MASK_SHOT | CONTENTS_BULLETS)


// gi.BoxEdicts() can return a list of either solid or trigger entities
// FIXME: eliminate AREA_ distinction?
#define AREA_SOLID      1
#define AREA_TRIGGERS   2


// plane_t structure
typedef struct cplane_s
{
	vec3_t  normal;
	float   dist;
	byte    type;           // for fast side tests
	byte    signbits;       // signx + (signy<<1) + (signz<<1)
	byte    pad[2];
} cplane_t;

// 0-2 are axial planes
#define PLANE_X         0
#define PLANE_Y         1
#define PLANE_Z         2

// 3-5 are non-axial planes snapped to the nearest
#define PLANE_ANYX      3
#define PLANE_ANYY      4
#define PLANE_ANYZ      5

// planes (x&~1) and (x&~1)+1 are always opposites

#define PLANE_NON_AXIAL 6

typedef struct csurface_s
{
	char			name[16];
	int         flags;
	int				value;
} csurface_t;

// a trace is returned when a box is swept through the world
typedef struct
{
	bool    allsolid;   // if true, plane is not valid
	bool    startsolid; // if true, the initial point was in a solid area
	float			fraction;   // time completed, 1.0 = didn't hit anything
	vec3_t			endpos;     // final position
	cplane_t		plane;      // surface normal at impact
	csurface_t		*surface;   // surface hit
	int         contents;   // contents on other side of surface hit
	struct edict_s  *ent;       // not set by CM_*() functions
} trace_t;

// pmove_state_t is the information necessary for client side movement
// prediction
typedef enum
{
	// can accelerate and turn
	PM_NORMAL,
	PM_SPECTATOR,
	// no acceleration or turning
	PM_DEAD,
	PM_GIB,     // different bounding box
	PM_FREEZE

	// Generations
	, PM_DUKE_FROZEN
} pmtype_t;

// pmove->pm_flags
#define PMF_DUCKED          1
#define PMF_JUMP_HELD       2
#define PMF_ON_GROUND       4
#define PMF_TIME_WATERJUMP  8   // pm_time is waterjump
#define PMF_TIME_LAND       16  // pm_time is time before rejump
#define PMF_TIME_TELEPORT   32  // pm_time is non-moving time
#define PMF_NO_PREDICTION   64  // temporarily disables prediction (used for grappling hook)
#define PMF_TELEPORT_BIT    128 // used by q2pro
#define PMF_AWAIT_JUMP      256 // Generations

// this structure needs to be communicated bit-accurate
// from the server to the client to guarantee that
// prediction stays in sync, so no floats are used.
// if any part of the game code modifies this struct, it
// will result in a prediction error of some degree.
typedef struct
{
	pmtype_t    pm_type;

	short		origin[3];      // 12.3
	short		velocity[3];    // 12.3
	short		pm_flags;       // ducked, jump_held, etc
	byte		pm_time;        // each unit = 8 ms
	byte		pm_time2;       // Generations; each unit = 8 ms
	short		gravity;
	short		delta_angles[3];    // add to command angles to get view direction
	// changed by spawns, rotating objects, and teleporters
} pmove_state_t;


//
// button bits
//
#define BUTTON_ATTACK       1
#define BUTTON_USE          2
#define BUTTON_ANY          128         // any key whatsoever


// usercmd_t is sent to the server each client frame
typedef struct usercmd_s
{
	byte    msec;
	byte    buttons;
	short   angles[3];
	short   forwardmove, sidemove, upmove;
	byte    impulse;        // remove?
} usercmd_t;


#define MAXTOUCH    32
typedef struct
{
	// state (in / out)
	pmove_state_t   s;

	// command (in)
	usercmd_t       cmd;
	bool        snapinitial;    // if s has been changed outside pmove

	// results (out)
	int         numtouch;
	struct edict_s  *touchents[MAXTOUCH];

	vec3_t      viewangles;         // clamped
	float       viewheight;

	vec3_t      mins, maxs;         // bounding box size

	struct edict_s  *groundentity;
	int         watertype;
	int         waterlevel;
	gametype_t	game; // Paril

	// callbacks to test the world
	trace_t (* q_gameabi trace)(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
	int (*pointcontents)(vec3_t point);
} pmove_t;


// entity_state_t->effects
// Effects are things handled on the client side (lights, particles, frame animations)
// that happen constantly on the given entity.
// An entity that has effects will be sent to the client
// even if it has a zero index model.
#define EF_ROTATE           0x00000001      // rotate (bonus items)
#define EF_GIB              0x00000002      // leave a trail
#define EF_BLASTER          0x00000008      // redlight + trail
#define EF_ROCKET           0x00000010      // redlight + trail
#define EF_GRENADE          0x00000020
#define EF_HYPERBLASTER     0x00000040
#define EF_BFG              0x00000080
#define EF_COLOR_SHELL      0x00000100
#define EF_POWERSCREEN      0x00000200
#define EF_ANIM01           0x00000400      // automatically cycle between frames 0 and 1 at 2 hz
#define EF_ANIM23           0x00000800      // automatically cycle between frames 2 and 3 at 2 hz
#define EF_ANIM_ALL         0x00001000      // automatically cycle through all frames at 2hz
#define EF_ANIM_ALLFAST     0x00002000      // automatically cycle through all frames at 10hz
#define EF_FLIES            0x00004000
#define EF_QUAD             0x00008000
#define EF_PENT             0x00010000
#define EF_TELEPORTER       0x00020000      // particle fountain
#define EF_FLAG1            0x00040000
#define EF_FLAG2            0x00080000
// RAFAEL
#define EF_IONRIPPER        0x00100000
#define EF_GREENGIB         0x00200000
#define EF_BLUEHYPERBLASTER 0x00400000
#define EF_SPINNINGLIGHTS   0x00800000
#define EF_PLASMA           0x01000000
#define EF_TRAP             0x02000000

//ROGUE
#define EF_TRACKER          0x04000000
#define EF_DOUBLE           0x08000000
#define EF_SPHERETRANS      0x10000000
#define EF_TAGTRAIL         0x20000000
#define EF_HALF_DAMAGE      0x40000000
#define EF_TRACKERTRAIL     0x80000000
//ROGUE

// Generations
#define EF_Q1_WIZ			EF_BLUEHYPERBLASTER
#define EF_Q1_VORE			EF_IONRIPPER
#define EF_Q1_HKNIGHT		EF_GREENGIB

// entity_state_t->renderfx flags
#define RF_MINLIGHT         1       // allways have some light (viewmodel)
#define RF_VIEWERMODEL      2       // don't draw through eyes, only mirrors
#define RF_WEAPONMODEL      4       // only draw through eyes
#define RF_FULLBRIGHT       8       // allways draw full intensity
#define RF_DEPTHHACK        16      // for view weapon Z crunching
#define RF_TRANSLUCENT      32
#define RF_FRAMELERP        64
#define RF_BEAM             128
#define RF_CUSTOMSKIN       256     // skin is an index in image_precache
#define RF_GLOW             512     // pulse lighting for bonus items
#define RF_SHELL_RED        1024
#define RF_SHELL_GREEN      2048
#define RF_SHELL_BLUE       4096

//ROGUE
#define RF_IR_VISIBLE       0x00008000      // 32768
#define RF_SHELL_DOUBLE     0x00010000      // 65536
#define RF_SHELL_HALF_DAM   0x00020000
#define RF_USE_DISGUISE     0x00040000
//ROGUE

// Paril
#define RF_SKIN_ANIM		0x00080000
#define RF_NOLERP			0x00100000
#define RF_ANIM_BOUNCE		0x00200000
#define RF_SPECTRE			0x00400000
#define RF_PROJECTILE		0x00800000
#define RF_NOCULL			0x01000000
#define RF_FROZEN			0x02000000
#define	RF_NO_BILLBOARD		0x04000000

// player_state_t->refdef flags
#define RDF_UNDERWATER      1       // warp the screen as apropriate
#define RDF_NOWORLDMODEL    2       // used for player configuration screen

//ROGUE
#define RDF_IRGOGGLES       4
#define RDF_UVGOGGLES       8
//ROGUE

// Generations
#define RDF_INVUL			16

//
// muzzle flashes / player effects
//
#define MZ_BLASTER          0
#define MZ_MACHINEGUN       1
#define MZ_SHOTGUN          2
#define MZ_CHAINGUN1        3
#define MZ_CHAINGUN2        4
#define MZ_CHAINGUN3        5
#define MZ_RAILGUN          6
#define MZ_ROCKET           7
#define MZ_GRENADE          8
#define MZ_LOGIN            9
#define MZ_LOGOUT           10
#define MZ_RESPAWN          11
#define MZ_BFG              12
#define MZ_SSHOTGUN         13
#define MZ_HYPERBLASTER     14
#define MZ_ITEMRESPAWN      15
// RAFAEL
#define MZ_IONRIPPER        16
#define MZ_BLUEHYPERBLASTER 17
#define MZ_PHALANX          18
#define MZ_SILENCED         128     // bit flag ORed with one of the above numbers

//ROGUE
#define MZ_ETF_RIFLE        30
#define MZ_UNUSED           31
#define MZ_SHOTGUN2         32
#define MZ_HEATBEAM         33
#define MZ_BLASTER2         34
#define MZ_TRACKER          35
#define MZ_NUKE1            36
#define MZ_NUKE2            37
#define MZ_NUKE4            38
#define MZ_NUKE8            39
//ROGUE

//
// monster muzzle flashes
//
#define MZ2_TANK_BLASTER_1              1
#define MZ2_TANK_BLASTER_2              2
#define MZ2_TANK_BLASTER_3              3
#define MZ2_TANK_MACHINEGUN_1           4
#define MZ2_TANK_MACHINEGUN_2           5
#define MZ2_TANK_MACHINEGUN_3           6
#define MZ2_TANK_MACHINEGUN_4           7
#define MZ2_TANK_MACHINEGUN_5           8
#define MZ2_TANK_MACHINEGUN_6           9
#define MZ2_TANK_MACHINEGUN_7           10
#define MZ2_TANK_MACHINEGUN_8           11
#define MZ2_TANK_MACHINEGUN_9           12
#define MZ2_TANK_MACHINEGUN_10          13
#define MZ2_TANK_MACHINEGUN_11          14
#define MZ2_TANK_MACHINEGUN_12          15
#define MZ2_TANK_MACHINEGUN_13          16
#define MZ2_TANK_MACHINEGUN_14          17
#define MZ2_TANK_MACHINEGUN_15          18
#define MZ2_TANK_MACHINEGUN_16          19
#define MZ2_TANK_MACHINEGUN_17          20
#define MZ2_TANK_MACHINEGUN_18          21
#define MZ2_TANK_MACHINEGUN_19          22
#define MZ2_TANK_ROCKET_1               23
#define MZ2_TANK_ROCKET_2               24
#define MZ2_TANK_ROCKET_3               25

#define MZ2_INFANTRY_MACHINEGUN_1       26
#define MZ2_INFANTRY_MACHINEGUN_2       27
#define MZ2_INFANTRY_MACHINEGUN_3       28
#define MZ2_INFANTRY_MACHINEGUN_4       29
#define MZ2_INFANTRY_MACHINEGUN_5       30
#define MZ2_INFANTRY_MACHINEGUN_6       31
#define MZ2_INFANTRY_MACHINEGUN_7       32
#define MZ2_INFANTRY_MACHINEGUN_8       33
#define MZ2_INFANTRY_MACHINEGUN_9       34
#define MZ2_INFANTRY_MACHINEGUN_10      35
#define MZ2_INFANTRY_MACHINEGUN_11      36
#define MZ2_INFANTRY_MACHINEGUN_12      37
#define MZ2_INFANTRY_MACHINEGUN_13      38

#define MZ2_SOLDIER_BLASTER_1           39
#define MZ2_SOLDIER_BLASTER_2           40
#define MZ2_SOLDIER_SHOTGUN_1           41
#define MZ2_SOLDIER_SHOTGUN_2           42
#define MZ2_SOLDIER_MACHINEGUN_1        43
#define MZ2_SOLDIER_MACHINEGUN_2        44

#define MZ2_GUNNER_MACHINEGUN_1         45
#define MZ2_GUNNER_MACHINEGUN_2         46
#define MZ2_GUNNER_MACHINEGUN_3         47
#define MZ2_GUNNER_MACHINEGUN_4         48
#define MZ2_GUNNER_MACHINEGUN_5         49
#define MZ2_GUNNER_MACHINEGUN_6         50
#define MZ2_GUNNER_MACHINEGUN_7         51
#define MZ2_GUNNER_MACHINEGUN_8         52
#define MZ2_GUNNER_GRENADE_1            53
#define MZ2_GUNNER_GRENADE_2            54
#define MZ2_GUNNER_GRENADE_3            55
#define MZ2_GUNNER_GRENADE_4            56

#define MZ2_CHICK_ROCKET_1              57

#define MZ2_FLYER_BLASTER_1             58
#define MZ2_FLYER_BLASTER_2             59

#define MZ2_MEDIC_BLASTER_1             60

#define MZ2_GLADIATOR_RAILGUN_1         61

#define MZ2_HOVER_BLASTER_1             62

#define MZ2_ACTOR_MACHINEGUN_1          63

#define MZ2_SUPERTANK_MACHINEGUN_1      64
#define MZ2_SUPERTANK_MACHINEGUN_2      65
#define MZ2_SUPERTANK_MACHINEGUN_3      66
#define MZ2_SUPERTANK_MACHINEGUN_4      67
#define MZ2_SUPERTANK_MACHINEGUN_5      68
#define MZ2_SUPERTANK_MACHINEGUN_6      69
#define MZ2_SUPERTANK_ROCKET_1          70
#define MZ2_SUPERTANK_ROCKET_2          71
#define MZ2_SUPERTANK_ROCKET_3          72

#define MZ2_BOSS2_MACHINEGUN_L1         73
#define MZ2_BOSS2_MACHINEGUN_L2         74
#define MZ2_BOSS2_MACHINEGUN_L3         75
#define MZ2_BOSS2_MACHINEGUN_L4         76
#define MZ2_BOSS2_MACHINEGUN_L5         77
#define MZ2_BOSS2_ROCKET_1              78
#define MZ2_BOSS2_ROCKET_2              79
#define MZ2_BOSS2_ROCKET_3              80
#define MZ2_BOSS2_ROCKET_4              81

#define MZ2_FLOAT_BLASTER_1             82

#define MZ2_SOLDIER_BLASTER_3           83
#define MZ2_SOLDIER_SHOTGUN_3           84
#define MZ2_SOLDIER_MACHINEGUN_3        85
#define MZ2_SOLDIER_BLASTER_4           86
#define MZ2_SOLDIER_SHOTGUN_4           87
#define MZ2_SOLDIER_MACHINEGUN_4        88
#define MZ2_SOLDIER_BLASTER_5           89
#define MZ2_SOLDIER_SHOTGUN_5           90
#define MZ2_SOLDIER_MACHINEGUN_5        91
#define MZ2_SOLDIER_BLASTER_6           92
#define MZ2_SOLDIER_SHOTGUN_6           93
#define MZ2_SOLDIER_MACHINEGUN_6        94
#define MZ2_SOLDIER_BLASTER_7           95
#define MZ2_SOLDIER_SHOTGUN_7           96
#define MZ2_SOLDIER_MACHINEGUN_7        97
#define MZ2_SOLDIER_BLASTER_8           98
#define MZ2_SOLDIER_SHOTGUN_8           99
#define MZ2_SOLDIER_MACHINEGUN_8        100

// --- Xian shit below ---
#define MZ2_MAKRON_BFG                  101
#define MZ2_MAKRON_BLASTER_1            102
#define MZ2_MAKRON_BLASTER_2            103
#define MZ2_MAKRON_BLASTER_3            104
#define MZ2_MAKRON_BLASTER_4            105
#define MZ2_MAKRON_BLASTER_5            106
#define MZ2_MAKRON_BLASTER_6            107
#define MZ2_MAKRON_BLASTER_7            108
#define MZ2_MAKRON_BLASTER_8            109
#define MZ2_MAKRON_BLASTER_9            110
#define MZ2_MAKRON_BLASTER_10           111
#define MZ2_MAKRON_BLASTER_11           112
#define MZ2_MAKRON_BLASTER_12           113
#define MZ2_MAKRON_BLASTER_13           114
#define MZ2_MAKRON_BLASTER_14           115
#define MZ2_MAKRON_BLASTER_15           116
#define MZ2_MAKRON_BLASTER_16           117
#define MZ2_MAKRON_BLASTER_17           118
#define MZ2_MAKRON_RAILGUN_1            119
#define MZ2_JORG_MACHINEGUN_L1          120
#define MZ2_JORG_MACHINEGUN_L2          121
#define MZ2_JORG_MACHINEGUN_L3          122
#define MZ2_JORG_MACHINEGUN_L4          123
#define MZ2_JORG_MACHINEGUN_L5          124
#define MZ2_JORG_MACHINEGUN_L6          125
#define MZ2_JORG_MACHINEGUN_R1          126
#define MZ2_JORG_MACHINEGUN_R2          127
#define MZ2_JORG_MACHINEGUN_R3          128
#define MZ2_JORG_MACHINEGUN_R4          129
#define MZ2_JORG_MACHINEGUN_R5          130
#define MZ2_JORG_MACHINEGUN_R6          131
#define MZ2_JORG_BFG_1                  132
#define MZ2_BOSS2_MACHINEGUN_R1         133
#define MZ2_BOSS2_MACHINEGUN_R2         134
#define MZ2_BOSS2_MACHINEGUN_R3         135
#define MZ2_BOSS2_MACHINEGUN_R4         136
#define MZ2_BOSS2_MACHINEGUN_R5         137

//ROGUE
#define MZ2_CARRIER_MACHINEGUN_L1       138
#define MZ2_CARRIER_MACHINEGUN_R1       139
#define MZ2_CARRIER_GRENADE             140
#define MZ2_TURRET_MACHINEGUN           141
#define MZ2_TURRET_ROCKET               142
#define MZ2_TURRET_BLASTER              143
#define MZ2_STALKER_BLASTER             144
#define MZ2_DAEDALUS_BLASTER            145
#define MZ2_MEDIC_BLASTER_2             146
#define MZ2_CARRIER_RAILGUN             147
#define MZ2_WIDOW_DISRUPTOR             148
#define MZ2_WIDOW_BLASTER               149
#define MZ2_WIDOW_RAIL                  150
#define MZ2_WIDOW_PLASMABEAM            151     // PMM - not used
#define MZ2_CARRIER_MACHINEGUN_L2       152
#define MZ2_CARRIER_MACHINEGUN_R2       153
#define MZ2_WIDOW_RAIL_LEFT             154
#define MZ2_WIDOW_RAIL_RIGHT            155
#define MZ2_WIDOW_BLASTER_SWEEP1        156
#define MZ2_WIDOW_BLASTER_SWEEP2        157
#define MZ2_WIDOW_BLASTER_SWEEP3        158
#define MZ2_WIDOW_BLASTER_SWEEP4        159
#define MZ2_WIDOW_BLASTER_SWEEP5        160
#define MZ2_WIDOW_BLASTER_SWEEP6        161
#define MZ2_WIDOW_BLASTER_SWEEP7        162
#define MZ2_WIDOW_BLASTER_SWEEP8        163
#define MZ2_WIDOW_BLASTER_SWEEP9        164
#define MZ2_WIDOW_BLASTER_100           165
#define MZ2_WIDOW_BLASTER_90            166
#define MZ2_WIDOW_BLASTER_80            167
#define MZ2_WIDOW_BLASTER_70            168
#define MZ2_WIDOW_BLASTER_60            169
#define MZ2_WIDOW_BLASTER_50            170
#define MZ2_WIDOW_BLASTER_40            171
#define MZ2_WIDOW_BLASTER_30            172
#define MZ2_WIDOW_BLASTER_20            173
#define MZ2_WIDOW_BLASTER_10            174
#define MZ2_WIDOW_BLASTER_0             175
#define MZ2_WIDOW_BLASTER_10L           176
#define MZ2_WIDOW_BLASTER_20L           177
#define MZ2_WIDOW_BLASTER_30L           178
#define MZ2_WIDOW_BLASTER_40L           179
#define MZ2_WIDOW_BLASTER_50L           180
#define MZ2_WIDOW_BLASTER_60L           181
#define MZ2_WIDOW_BLASTER_70L           182
#define MZ2_WIDOW_RUN_1                 183
#define MZ2_WIDOW_RUN_2                 184
#define MZ2_WIDOW_RUN_3                 185
#define MZ2_WIDOW_RUN_4                 186
#define MZ2_WIDOW_RUN_5                 187
#define MZ2_WIDOW_RUN_6                 188
#define MZ2_WIDOW_RUN_7                 189
#define MZ2_WIDOW_RUN_8                 190
#define MZ2_CARRIER_ROCKET_1            191
#define MZ2_CARRIER_ROCKET_2            192
#define MZ2_CARRIER_ROCKET_3            193
#define MZ2_CARRIER_ROCKET_4            194
#define MZ2_WIDOW2_BEAMER_1             195
#define MZ2_WIDOW2_BEAMER_2             196
#define MZ2_WIDOW2_BEAMER_3             197
#define MZ2_WIDOW2_BEAMER_4             198
#define MZ2_WIDOW2_BEAMER_5             199
#define MZ2_WIDOW2_BEAM_SWEEP_1         200
#define MZ2_WIDOW2_BEAM_SWEEP_2         201
#define MZ2_WIDOW2_BEAM_SWEEP_3         202
#define MZ2_WIDOW2_BEAM_SWEEP_4         203
#define MZ2_WIDOW2_BEAM_SWEEP_5         204
#define MZ2_WIDOW2_BEAM_SWEEP_6         205
#define MZ2_WIDOW2_BEAM_SWEEP_7         206
#define MZ2_WIDOW2_BEAM_SWEEP_8         207
#define MZ2_WIDOW2_BEAM_SWEEP_9         208
#define MZ2_WIDOW2_BEAM_SWEEP_10        209
#define MZ2_WIDOW2_BEAM_SWEEP_11        210

// ROGUE

extern  const vec3_t monster_flash_offset [];


// temp entity events
//
// Temp entity events are for things that happen
// at a location seperate from any existing entity.
// Temporary entity messages are explicitly constructed
// and broadcast.
typedef enum
{
	TE_GUNSHOT,
	TE_BLOOD,
	TE_BLASTER,
	TE_RAILTRAIL,
	TE_SHOTGUN,
	TE_EXPLOSION1,
	TE_EXPLOSION2,
	TE_ROCKET_EXPLOSION,
	TE_GRENADE_EXPLOSION,
	TE_SPARKS,
	TE_SPLASH,
	TE_BUBBLETRAIL,
	TE_SCREEN_SPARKS,
	TE_SHIELD_SPARKS,
	TE_BULLET_SPARKS,
	TE_LASER_SPARKS,
	TE_PARASITE_ATTACK,
	TE_ROCKET_EXPLOSION_WATER,
	TE_GRENADE_EXPLOSION_WATER,
	TE_MEDIC_CABLE_ATTACK,
	TE_BFG_EXPLOSION,
	TE_BFG_BIGEXPLOSION,
	TE_BOSSTPORT,           // used as '22' in a map, so DON'T RENUMBER!!!
	TE_BFG_LASER,
	TE_GRAPPLE_CABLE,
	TE_WELDING_SPARKS,
	TE_GREENBLOOD,
	TE_BLUEHYPERBLASTER,
	TE_PLASMA_EXPLOSION,
	TE_TUNNEL_SPARKS,
	//ROGUE
	TE_BLASTER2,
	TE_RAILTRAIL2,
	TE_FLAME,
	TE_LIGHTNING,
	TE_DEBUGTRAIL,
	TE_PLAIN_EXPLOSION,
	TE_FLASHLIGHT,
	TE_FORCEWALL,
	TE_HEATBEAM,
	TE_MONSTER_HEATBEAM,
	TE_STEAM,
	TE_BUBBLETRAIL2,
	TE_MOREBLOOD,
	TE_HEATBEAM_SPARKS,
	TE_HEATBEAM_STEAM,
	TE_CHAINFIST_SMOKE,
	TE_ELECTRIC_SPARKS,
	TE_TRACKER_EXPLOSION,
	TE_TELEPORT_EFFECT,
	TE_DBALL_GOAL,
	TE_WIDOWBEAMOUT,
	TE_NUKEBLAST,
	TE_WIDOWSPLASH,
	TE_EXPLOSION1_BIG,
	TE_EXPLOSION1_NP,
	TE_FLECHETTE,
	//ROGUE

	// Generations
	TE_Q1_GUNSHOT,
	TE_Q1_BLOOD,
	TE_Q1_SUPERSPIKE,
	TE_Q1_SPIKE,
	TE_Q1_EXPLODE,
	TE_Q1_LIGHTNING1,
	TE_Q1_LIGHTNING2,
	TE_Q1_LIGHTNING3,
	TE_Q1_LIGHTNINGBLOOD,
	TE_Q1_WIZSPIKE,
	TE_Q1_KNIGHTSPIKE,
	TE_Q1_LAVASPLASH,
	TE_Q1_TAREXPLOSION,

	TE_DOOM_GUNSHOT,
	TE_DOOM_EXPLODE,
	TE_DOOM_PLASMA,
	TE_DOOM_BLOOD,
	TE_DOOM_IMP_BOOM,
	TE_DOOM_CACO_BOOM,
	TE_DOOM_BSPI_BOOM,
	TE_DOOM_PUFF,
	TE_DOOM_FBXP_BOOM,
	TE_DOOM_BOSS_BOOM,

	TE_DUKE_GUNSHOT,
	TE_DUKE_BLOOD,
	TE_DUKE_BLOOD_SPLAT,
	TE_DUKE_EXPLODE,
	TE_DUKE_EXPLODE_SMALL,
	TE_DUKE_EXPLODE_PIPE,
	TE_DUKE_FREEZE_HIT,
	TE_DUKE_GLASS,

	TE_DAMAGE_DIRECTION,

	TE_NUM_ENTITIES
} temp_event_t;

#define SPLASH_UNKNOWN      0
#define SPLASH_SPARKS       1
#define SPLASH_BLUE_WATER   2
#define SPLASH_BROWN_WATER  3
#define SPLASH_SLIME        4
#define SPLASH_LAVA         5
#define SPLASH_BLOOD        6


// sound channels
// channel 0 never willingly overrides
// other channels (1-7) allways override a playing sound on that channel
#define CHAN_AUTO               0
#define CHAN_WEAPON             1
#define CHAN_VOICE              2
#define CHAN_ITEM               3
#define CHAN_BODY               4
// Paril
#define CHAN_WEAPON2			8
// modifier flags
#define CHAN_NO_PHS_ADD         8   // send to all clients, not just ones in PHS (ATTN 0 will also do this)
#define CHAN_RELIABLE           16  // send by reliable message, not datagram
// Paril
#define	CHAN_NO_EAVESDROP		32


// sound attenuation values
#define ATTN_NONE               0   // full volume the entire level
#define ATTN_NORM               1
#define ATTN_IDLE               2
#define ATTN_STATIC             3   // diminish very rapidly with distance

enum
{
	IT_Q1_SHOTGUN			= 1 << 0,
	IT_Q1_SSHOTGUN			= 1 << 1,
	IT_Q1_NAILGUN			= 1 << 2,
	IT_Q1_SNAILGUN			= 1 << 3,
	IT_Q1_GLAUNCHER			= 1 << 4,
	IT_Q1_RLAUNCHER			= 1 << 5,
	IT_Q1_LIGHTNING			= 1 << 6,
	IT_Q1_QUAD				= 1 << 7,
	IT_Q1_INVUL				= 1 << 8,
	IT_Q1_SUIT				= 1 << 9,
	IT_Q1_INVIS				= 1 << 10
};

enum
{
	IT_DOOM_PISTOL			= 1 << 1,
	IT_DOOM_SHOTGUNS		= 1 << 2,
	IT_DOOM_CHAINGUN		= 1 << 3,
	IT_DOOM_ROCKET			= 1 << 4,
	IT_DOOM_PLASMA			= 1 << 5,
	IT_DOOM_BFG				= 1 << 6
};

enum
{
	IT_DUKE_PISTOL			= 1 << 2,
	IT_DUKE_SHOTGUN			= 1 << 3,
	IT_DUKE_CANNON			= 1 << 4,
	IT_DUKE_RPG				= 1 << 5,
	IT_DUKE_PIPE			= 1 << 6,
	IT_DUKE_SHRINKER		= 1 << 7,
	IT_DUKE_DEVASTATOR		= 1 << 8,
	IT_DUKE_TRIPWIRE		= 1 << 9,
	IT_DUKE_FREEZETHROWER	= 1 << 10,

	IT_DUKE_INDEX_PISTOL		= 2,
	IT_DUKE_INDEX_SHOTGUN		= 3,
	IT_DUKE_INDEX_CANNON		= 4,
	IT_DUKE_INDEX_RPG			= 5,
	IT_DUKE_INDEX_PIPE			= 6,
	IT_DUKE_INDEX_SHRINKER		= 7,
	IT_DUKE_INDEX_DEVASTATOR	= 8,
	IT_DUKE_INDEX_TRIPWIRE		= 9,
	IT_DUKE_INDEX_FREEZETHROWER	= 10
};

// Doom face stuff
#define ST_NUMPAINFACES		5
#define ST_NUMSTRAIGHTFACES	3
#define ST_NUMTURNFACES		2
#define ST_NUMSPECIALFACES	3

#define ST_FACESTRIDE		(ST_NUMSTRAIGHTFACES + ST_NUMTURNFACES + ST_NUMSPECIALFACES)

#define ST_NUMEXTRAFACES	2

#define ST_NUMFACES			(ST_FACESTRIDE * ST_NUMPAINFACES) + ST_NUMEXTRAFACES

#define ST_TURNOFFSET		ST_NUMSTRAIGHTFACES
#define ST_OUCHOFFSET		ST_TURNOFFSET + ST_NUMTURNFACES
#define ST_EVILGRINOFFSET	ST_OUCHOFFSET + 1
#define ST_RAMPAGEOFFSET	ST_EVILGRINOFFSET + 1
#define ST_GODFACE			ST_NUMPAINFACES*ST_FACESTRIDE
#define ST_DEADFACE			ST_GODFACE+1

// dmflags->value flags
#define DF_NO_HEALTH        0x00000001  // 1
#define DF_NO_ITEMS         0x00000002  // 2
#define DF_WEAPONS_STAY     0x00000004  // 4
#define DF_NO_FALLING       0x00000008  // 8
#define DF_INSTANT_ITEMS    0x00000010  // 16
#define DF_SAME_LEVEL       0x00000020  // 32
#define DF_SKINTEAMS        0x00000040  // 64
#define DF_MODELTEAMS       0x00000080  // 128
#define DF_NO_FRIENDLY_FIRE 0x00000100  // 256
#define DF_SPAWN_FARTHEST   0x00000200  // 512
#define DF_FORCE_RESPAWN    0x00000400  // 1024
#define DF_NO_ARMOR         0x00000800  // 2048
#define DF_ALLOW_EXIT       0x00001000  // 4096
#define DF_INFINITE_AMMO    0x00002000  // 8192
#define DF_QUAD_DROP        0x00004000  // 16384
#define DF_FIXED_FOV        0x00008000  // 32768

// RAFAEL
#define DF_QUADFIRE_DROP    0x00010000  // 65536

//ROGUE
#define DF_NO_MINES         0x00020000
#define DF_NO_STACK_DOUBLE  0x00040000
#define DF_NO_NUKES         0x00080000
#define DF_NO_SPHERES       0x00100000
//ROGUE

/*
ROGUE - VERSIONS
1234    08/13/1998      Activision
1235    08/14/1998      Id Software
1236    08/15/1998      Steve Tietze
1237    08/15/1998      Phil Dobranski
1238    08/15/1998      John Sheley
1239    08/17/1998      Barrett Alexander
1230    08/17/1998      Brandon Fish
1245    08/17/1998      Don MacAskill
1246    08/17/1998      David "Zoid" Kirsch
1247    08/17/1998      Manu Smith
1248    08/17/1998      Geoff Scully
1249    08/17/1998      Andy Van Fossen
1240    08/20/1998      Activision Build 2
1256    08/20/1998      Ranger Clan
1257    08/20/1998      Ensemble Studios
1258    08/21/1998      Robert Duffy
1259    08/21/1998      Stephen Seachord
1250    08/21/1998      Stephen Heaslip
1267    08/21/1998      Samir Sandesara
1268    08/21/1998      Oliver Wyman
1269    08/21/1998      Steven Marchegiano
1260    08/21/1998      Build #2 for Nihilistic
1278    08/21/1998      Build #2 for Ensemble

9999    08/20/1998      Internal Use
*/
#define ROGUE_VERSION_ID        1278

#define ROGUE_VERSION_STRING    "08/21/1998 Beta 2 for Ensemble"

// ROGUE

/*
==========================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

==========================================================
*/

// default server FPS
#define BASE_FRAMERATE          10
#define BASE_FRAMETIME          ((1.0 / BASE_FRAMERATE) * 1000)
#define BASE_1_FRAMETIME        (1.0 / BASE_FRAMETIME)
#define BASE_FRAMETIME_1000     (BASE_FRAMETIME / 1000)

static inline short ANGLE2SHORT(const float x)
{
	return (int)(x * 65536 / 360) & 65535;
}

static inline float SHORT2ANGLE(const short x)
{
	return x * (360.0f / 65536);
}


//
// config strings are a general means of communication from
// the server to all connected clients.
// Each config string can be at most MAX_QPATH characters.
//
#define CS_NAME             0
#define CS_CDTRACK          1
#define CS_SKY              2
#define CS_SKYAXIS          3       // %f %f %f format
#define CS_SKYROTATE        4
#define CS_STATUSBAR        5       // display program string
#define CS_GAMEMODE			6		// Generations

#define CS_AIRACCEL         29      // air acceleration control
#define CS_MAXCLIENTS       30
#define CS_MAPCHECKSUM      31      // for catching cheater maps

// Generations
#define CS_PRECACHE         32
#define CS_LIGHTS           (CS_PRECACHE+MAX_PRECACHE)
#define CS_ITEMS            (CS_LIGHTS+MAX_LIGHTSTYLES)
// Generations
#define CS_PLAYERSKINS      (CS_ITEMS+ITI_TOTAL)
#define CS_GENERAL          (CS_PLAYERSKINS+MAX_CLIENTS)
#define MAX_CONFIGSTRINGS   (CS_GENERAL+MAX_GENERAL)

// Some mods actually exploit CS_STATUSBAR to take space up to CS_AIRACCEL
#define CS_SIZE(cs) \
	((cs) >= CS_STATUSBAR && (cs) < CS_AIRACCEL ? \
		MAX_QPATH * (CS_AIRACCEL - (cs)) : MAX_QPATH)

typedef enum
{
	GAMEMODE_SINGLEPLAYER,
	GAMEMODE_COOP,
	GAMEMODE_INVASION,
	GAMEMODE_DEATHMATCH
} gamemode_t;

//==============================================


// entity_state_t->event values
// ertity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.
// All muzzle flashes really should be converted to events...
typedef enum
{
	EV_NONE,
	EV_ITEM_RESPAWN,
	EV_FOOTSTEP,
	EV_FALLSHORT,
	EV_FALL,
	EV_FALLFAR,
	EV_PLAYER_TELEPORT,
	EV_OTHER_TELEPORT
} entity_event_t;

// entity_state_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
typedef struct entity_state_s
{
	int     number;         // edict index

	vec3_t  origin;
	vec3_t  angles;
	vec3_t  old_origin;     // for lerping
	q_modelhandle     modelindex;
	q_modelhandle     modelindex2, modelindex3, modelindex4;  // weapons, CTF flags, etc
	int     frame;
	int     skinnum;
	unsigned int        effects;        // PGM - we're filling it, so it needs to be unsigned
	int     renderfx;
	int     solid;          // for client side prediction, 8*(bits 0-4) is x/y radius
	// 8*(bits 5-9) is z down distance, 8(bits10-15) is z up
	// gi.linkentity sets this properly
	q_soundhandle     sound;          // for looping sounds, to guarantee shutoff
	int     event;          // impulse events -- muzzle flashes, footsteps, etc
	// events only go out for a single frame, they
	// are automatically cleared each frame

	// Paril
	gametype_t     game;
	int		clip_contents;
} entity_state_t;

//==============================================

typedef enum
{
	PEV_NONE		= 0,
	PEV_DAMAGED		= 1 << 0,
	PEV_ITEM		= 1 << 1
} player_event_t;

typedef struct
{
	vec3_t			angles;
	vec3_t			offset;
	q_modelhandle	index;
	uint16_t		frame;
} player_gun_t;

typedef enum
{
	GUN_MAIN,
	GUN_OFFHAND,

	MAX_PLAYER_GUNS
} gunindex_e;

// player_stats_t is a structure that defines the stats.
// this is sent delta + run-length encoded.
// NOTE TO BE SURE IT MATCHES SCR_ParseLayoutStatOffset
typedef struct
{
	// stats used by every class
	q_imagehandle	ammo_icon;
	q_imagehandle	armor_icon;

	int16_t		health;
	uint16_t	ammo;
	uint16_t	armor;

	uint16_t	frags;
	uint16_t	chase;

	uint8_t		selected_item;
	union
	{
		struct
		{
			uint8_t		spectator : 1;
			uint8_t		layouts : 2;
			uint8_t		flashes : 2;
		};

		uint8_t	visual_bits;
	};

	// class-specific stats
	union
	{
		struct
		{
			q_imagehandle	health_icon;
			q_imagehandle	pickup_icon;
			q_imagehandle	timer_icon;
			q_imagehandle	selected_icon;
			q_imagehandle	help_icon;
			uint16_t	timer;
			uint16_t	pickup_string;
		} q2;

		struct
		{
			uint16_t items : 10;
			uint16_t ammo_shells : 9;
			uint16_t ammo_nails : 9;
			uint16_t ammo_rockets : 9;
			uint16_t ammo_cells : 9;
			uint8_t face_anim : 3;
			uint8_t	cur_weap;
		} q1;

		struct
		{
			uint16_t ammo_bullets : 9;
			uint16_t ammo_shells : 9;
			uint16_t ammo_rockets : 9;
			uint16_t ammo_cells : 9;
			uint8_t weapons : 7;
			uint8_t face : 6;
			uint16_t max_ammo_bullets : 9;
			uint16_t max_ammo_shells : 9;
			uint16_t max_ammo_rockets : 9;
			uint16_t max_ammo_cells : 9;
		} doom;

		struct
		{
			uint16_t weapons : 11;
			uint8_t selected_weapon : 4;
			uint16_t ammo_clip : 9;
			uint16_t ammo_shells : 9;
			uint16_t ammo_cannon : 9;
			uint16_t ammo_rpg : 9;
			uint16_t ammo_pipebombs : 9;
			uint16_t ammo_shrinker : 9;
			uint16_t ammo_devastator : 9;
			uint16_t ammo_tripwire : 9;
			uint16_t ammo_freezer : 9;
			uint16_t max_ammo_clip : 9;
			uint16_t max_ammo_shells : 9;
			uint16_t max_ammo_cannon : 9;
			uint16_t max_ammo_rpg : 9;
			uint16_t max_ammo_pipebombs : 9;
			uint16_t max_ammo_shrinker : 9;
			uint16_t max_ammo_devastator : 9;
			uint16_t max_ammo_tripwire : 9;
			uint16_t max_ammo_freezer : 9;
		} duke;
	};
} player_stats_t;

// player_state_t is the information needed in addition to pmove_state_t
// to rendered a view.  There will only be 10 player_state_t sent each second,
// but the number of pmove_state_t changes will be reletive to client
// frame rates
typedef struct
{
	pmove_state_t   pmove;      // for prediction

	// these fields do not need to be communicated bit-precise

	vec3_t			viewangles;     // for fixed views
	vec3_t			viewoffset;     // add to pmovestate->origin
	vec3_t			kick_angles;    // add to view direction to get render angles
	// set by weapon kicks, pain effects, etc

	// Generations
	player_gun_t	guns[MAX_PLAYER_GUNS];

	float       	blend[4];       // rgba full screen effect

	float			fov;            // horizontal field of view

	int         	rdflags;        // refdef flags

	// Generations
	player_stats_t	stats;       // fast status bar updates

	player_event_t	view_events;
} player_state_t;

// hash stuff
unsigned Com_HashString(const char *s, unsigned size);
unsigned Com_HashStringLen(const char *s, size_t len, unsigned size);

// Navigation stuff
#define MAX_NODES 4096
#define MAX_NODE_CONNECTIONS 16

typedef enum
{
	NAV_NODE_NORMAL,
	NAV_NODE_JUMP,
	NAV_NODE_PLATFORM,

	NAV_NODE_INVALID = 0xFFFFFFFFu
} nav_node_type_e;

typedef uint32_t nav_node_id;

typedef double(*nav_igator_heuristic_func)(nav_node_id a, nav_node_id b);
typedef bool(*nav_igator_node_func)(nav_node_id node);

typedef struct nav_igator_s nav_igator_t;

// FILE SYSTEM
// bits 0 - 1, enum
#define FS_MODE_APPEND          0x00000000
#define FS_MODE_READ            0x00000001
#define FS_MODE_WRITE           0x00000002
#define FS_MODE_RDWR            0x00000003
#define FS_MODE_MASK            0x00000003

// bits 2 - 3, enum
#define FS_BUF_DEFAULT          0x00000000
#define FS_BUF_FULL             0x00000004
#define FS_BUF_LINE             0x00000008
#define FS_BUF_NONE             0x0000000c
#define FS_BUF_MASK             0x0000000c

// bits 4 - 5, enum
#define FS_TYPE_ANY             0x00000000
#define FS_TYPE_REAL            0x00000010
#define FS_TYPE_PAK             0x00000020
#define FS_TYPE_RESERVED        0x00000030
#define FS_TYPE_MASK            0x00000030

// bits 6 - 7, flag
#define FS_PATH_ANY             0x00000000
#define FS_PATH_BASE            0x00000040
#define FS_PATH_GAME            0x00000080
#define FS_PATH_MASK            0x000000c0

// bits 8 - 12, flag
#define FS_SEARCH_BYFILTER      0x00000100
#define FS_SEARCH_SAVEPATH      0x00000200
#define FS_SEARCH_EXTRAINFO     0x00000400
#define FS_SEARCH_STRIPEXT      0x00000800
#define FS_SEARCH_DIRSONLY      0x00001000
#define FS_SEARCH_MASK          0x00001f00

// bits 8 - 11, flag
#define FS_FLAG_GZIP            0x00000100
#define FS_FLAG_EXCL            0x00000200
#define FS_FLAG_TEXT            0x00000400
#define FS_FLAG_DEFLATE         0x00000800

#endif // SHARED_H
